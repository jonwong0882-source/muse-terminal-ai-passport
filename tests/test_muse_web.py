"""Exercise the Python browser adapter against a deterministic Muse-like page."""
import asyncio
import sys
import unittest
from pathlib import Path
from playwright.async_api import async_playwright
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from muse_web import WebSession,changed_reply,readiness,validate_url

URL='https://muse.ai/thread/test-session'
HTML='''<!doctype html><meta charset="utf-8"><textarea aria-label="Message"></textarea>
<button aria-label="Send">Send</button><button aria-label="Stop" hidden>Stop</button>
<div data-hatch-assistant-message-body><div data-hatch-markdown-streaming="false">old answer</div></div>
<script>
let sends=0;
document.querySelector('[aria-label="Send"]').onclick=()=>{
  sends++;
  const stop=document.querySelector('[aria-label="Stop"]');
  stop.hidden=false;
  setTimeout(()=>{
    const body=document.createElement('div');
    body.setAttribute('data-hatch-assistant-message-body','');
    body.innerHTML='<div data-hatch-markdown-streaming="true">新的中文回复</div>';
    document.body.append(body);
    document.querySelector('textarea').value='';
  },100);
  setTimeout(()=>{stop.hidden=true;document.querySelectorAll('[data-hatch-markdown-streaming]')[1].setAttribute('data-hatch-markdown-streaming','false');},350);
};
</script>'''

class Pure(unittest.TestCase):
    def test_url_and_old_message_exclusion(self):
        self.assertEqual(validate_url(URL),URL)
        with self.assertRaises(ValueError):validate_url('https://evil.example/thread/test')
        before={'messages':['old answer']}
        self.assertEqual(changed_reply(before,{'messages':['old answer','new']}),'new')
        self.assertEqual(changed_reply(before,{'messages':['old answer']}),'')
        self.assertEqual(changed_reply(before,{'messages':['updated']}),'updated')
    def test_readiness(self):
        base={'url':URL,'composer':True,'disabled':False,'draft':False,'generating':False,'dialog':False,'messages':['old']}
        self.assertEqual(readiness(base,URL),'ready')
        self.assertEqual(readiness({**base,'generating':True},URL),'generating')
        self.assertEqual(readiness({**base,'dialog':True},URL),'dialog')

class Browser(unittest.IsolatedAsyncioTestCase):
    async def test_single_submit_and_completed_reply(self):
        async with async_playwright() as playwright:
            browser=await playwright.chromium.launch(channel='chrome',headless=True)
            context=await browser.new_context()
            await context.route('https://muse.ai/**',lambda route:route.fulfill(status=200,body=HTML,content_type='text/html'))
            page=await context.new_page()
            await page.goto(URL)
            session=WebSession(page,URL)
            self.assertEqual(readiness(await session.snapshot(),URL),'ready')
            reply=await session.ask('测试指令',timeout=8)
            self.assertEqual(reply,'新的中文回复')
            self.assertEqual(await page.evaluate('sends'),1)
            await context.close()
            await browser.close()

if __name__=='__main__':unittest.main()
