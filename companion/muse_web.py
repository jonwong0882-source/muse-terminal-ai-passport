"""Drive the user's dedicated Muse web conversation through its visible UI.

Only the exact configured conversation is used. A command is clicked once and
never retried; approvals and sign-in remain in the browser for the user.
"""
import asyncio
import os
from pathlib import Path
from urllib.parse import urlparse
from playwright.async_api import async_playwright

SNAPSHOT_JS = r"""() => {
  const visible = e => {
    if (!e || e.disabled || e.closest('[hidden],[aria-hidden="true"]')) return false;
    const style = getComputedStyle(e);
    return style.display !== 'none' && style.visibility !== 'hidden'
      && e.getClientRects().length > 0;
  };
  const box = document.querySelector('textarea[aria-label="Message"]');
  const bodies = [...document.querySelectorAll('[data-hatch-assistant-message-body]')];
  const text = body => {
    const parts = [...body.querySelectorAll('[data-hatch-markdown-streaming]')];
    const rendered = parts.map(e => (e.innerText || e.textContent || '').trim())
      .filter(Boolean).join('\n');
    return rendered || (body.innerText || body.textContent || '').trim();
  };
  return {
    url: location.href,
    composer: !!box,
    disabled: !!box?.disabled,
    draft: !!box?.value?.trim(),
    generating: [...document.querySelectorAll('button[aria-label="Stop"]')].some(visible),
    dialog: [...document.querySelectorAll('[role="dialog"]')].some(visible),
    messages: bodies.map(text),
    streaming: !!bodies.at(-1)?.querySelector('[data-hatch-markdown-streaming="true"]')
  };
}"""

def validate_url(url):
    parsed = urlparse(url)
    if parsed.scheme != 'https' or parsed.netloc != 'muse.ai' or not parsed.path.startswith('/thread/') or parsed.query or parsed.fragment:
        raise ValueError('Muse URL must be an exact https://muse.ai/thread/... conversation')
    return url

def readiness(snapshot, target):
    if snapshot['url'] != target:
        return 'sign_in_or_wrong_page'
    if not snapshot['composer']:
        return 'no_composer'
    if snapshot['disabled']:
        return 'composer_disabled'
    if snapshot['draft']:
        return 'draft'
    if snapshot['generating']:
        return 'generating'
    if snapshot['dialog']:
        return 'dialog'
    if not snapshot['messages']:
        return 'no_assistant_message'
    return 'ready'

def changed_reply(before, current):
    old = before['messages']
    new = current['messages']
    for index in range(len(new) - 1, -1, -1):
        if new[index] and (index >= len(old) or new[index] != old[index]):
            return new[index]
    return ''

class WebSession:
    def __init__(self, page, target):
        self.page = page
        self.target = validate_url(target)

    async def snapshot(self):
        return await self.page.evaluate(SNAPSHOT_JS)

    async def ask(self, text, timeout=145):
        before = await self.snapshot()
        if readiness(before, self.target) != 'ready':
            raise RuntimeError('Muse 网页未就绪，请检查 Mac 上的专用窗口。')
        await self.page.locator('textarea[aria-label="Message"]').fill(text)
        # Click exactly once. If the acknowledgement is ambiguous, do not
        # submit again because Muse may already be working on the request.
        await self.page.locator('button[aria-label="Send"]:not([disabled])').click(timeout=4000)
        loop = asyncio.get_running_loop()
        deadline = loop.time() + timeout
        last = ''
        stable_at = loop.time()
        while loop.time() < deadline:
            await asyncio.sleep(.5)
            current = await self.snapshot()
            if current['url'] != self.target:
                raise RuntimeError('Muse 会话页面已切换，请在 Mac 检查；不会自动重发。')
            if not current['composer']:
                raise RuntimeError('Muse 页面已退出登录，请在 Mac 检查；不会自动重发。')
            candidate = changed_reply(before, current)
            if candidate != last:
                last = candidate
                stable_at = loop.time()
            if last and not current['generating'] and loop.time() - stable_at > (5 if current['streaming'] else 2):
                return last
        raise RuntimeError('Muse 回复等待超时。请在 Mac 查看，不会自动重发。')

def clear_stale_profile_lock(profile):
    """Drop a SingletonLock left behind by a browser that already exited.

    If Chrome dies mid-launch (for example while Muse is redirecting through
    its anti-bot challenge) it can leave this symlink pointing at a dead pid.
    Every later launch then fails, muse_ready stays false forever, and the
    terminal only ever shows "check the Mac bridge and Muse page".
    """
    lock = Path(profile) / 'SingletonLock'
    try:
        if not lock.is_symlink():
            return
        pid = os.readlink(lock).rsplit('-', 1)[-1]
        if not pid.isdigit():
            return
        try:
            os.kill(int(pid), 0)
            return
        except ProcessLookupError:
            pass
        except PermissionError:
            return
        lock.unlink()
        print('Cleared stale browser profile lock (pid %s).' % pid, flush=True)
    except OSError:
        pass


class MuseWeb:
    def __init__(self, target, profile):
        self.target = validate_url(target)
        self.profile = Path(profile)
        self.page = None
        self.context = None
        self.reason = 'starting'
        self.online = False
        self.lock = asyncio.Lock()

    async def run(self):
        self.profile.mkdir(parents=True, exist_ok=True)
        os.chmod(self.profile, 0o700)
        while True:
            clear_stale_profile_lock(self.profile)
            try:
                async with async_playwright() as playwright:
                    self.context = await playwright.chromium.launch_persistent_context(
                        str(self.profile), channel='chrome', headless=False,
                        no_viewport=True, accept_downloads=False,
                        args=['--no-first-run'])
                    self.page = self.context.pages[0] if self.context.pages else await self.context.new_page()
                    await self.page.goto(self.target, wait_until='domcontentloaded', timeout=30000)
                    while True:
                        if self.page.is_closed():
                            self.page = await self.context.new_page()
                            await self.page.goto(self.target, wait_until='domcontentloaded', timeout=30000)
                        if not self.lock.locked():
                            state = await WebSession(self.page, self.target).snapshot()
                            self.reason = readiness(state, self.target)
                            self.online = self.reason == 'ready'
                        await asyncio.sleep(1)
            except asyncio.CancelledError:
                raise
            except Exception as error:
                self.online = False
                self.reason = 'browser_' + type(error).__name__
                print('Muse browser unavailable:', type(error).__name__, flush=True)
            finally:
                self.online = False
                self.page = None
                if self.context:
                    try:
                        await self.context.close()
                    except Exception:
                        pass
                self.context = None
            await asyncio.sleep(5)

    async def ask(self, text):
        async with self.lock:
            if not self.online or self.page is None:
                raise RuntimeError('Muse 网页未就绪，请检查 Mac 上的专用窗口。')
            self.online = False
            self.reason = 'busy'
            try:
                return await WebSession(self.page, self.target).ask(text)
            finally:
                self.online = False
                self.reason = 'checking'
