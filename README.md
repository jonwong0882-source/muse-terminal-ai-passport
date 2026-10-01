[简体中文](README.zh_CN.md)

# Muse Pocket Companion: beginner setup

Turn AI Passport into a portable voice terminal for **your own Muse conversation**. Press OK to record, review the recognized words, and send them to Muse. Replies appear as text and are not read aloud. Passport connects by Wi-Fi to an awake Mac, where Python controls a dedicated Chrome window. No Chrome extension is needed.

If you use a local AI agent, copy the [agent installation prompt](docs/agent-install-prompt.md). You must still sign in to Muse yourself, enter your Wi-Fi password privately, and approve flashing before it happens.

## Before you start

- AI Passport, a USB **data** cable, a Mac, and Chrome. Use a reasonably recent Chrome (this project was validated on 154); the bridge drives the Chrome you already have and needs neither <code>playwright install</code> nor an extra Chromium download.
- Python 3.11 or later from the [official macOS downloads](https://www.python.org/downloads/macos/). Check with <code>python3.13 --version</code> in Terminal, substituting the version you actually installed. **The <code>python3</code> bundled with macOS is usually 3.9 and will not work**; wherever a command below names an interpreter, use your own 3.11+ build.
- Your Muse account and conversation URL in the form <code>https://muse.ai/thread/...</code>. Keep the private URL, password, and verification codes private.
- Mac and Passport on the same local network, with Passport using **2.4 GHz** Wi-Fi only (5 GHz is not supported). Keep the Mac awake; the network must not isolate Wi-Fi clients, and the firewall must permit TCP 18766 for the bridge.

The first run downloads Python dependencies and a local speech model. Recognition happens on the Mac; only text you confirm is sent to Muse. If the model download stalls, run <code>export HF_ENDPOINT=https://hf-mirror.com</code> before starting the bridge.

## 1. Download

From this repository's [latest Release](https://github.com/jonwong0882-source/muse-terminal-ai-passport/releases/latest), download and unzip <code>muse-terminal-community-package.zip</code>. It contains <code>firmware</code>, <code>source</code>, and <code>docs</code>. The automatic “Source code” ZIP does not include matching debug artifacts; use the release package for installation.

Open Terminal, type <code>cd </code> with a trailing space, drag the extracted **source folder** into Terminal, and press Return. Run the remaining commands from there.

## 2. Install and sign in to the Mac bridge

Run these commands one at a time:

~~~bash
python3.13 -m venv "$HOME/Library/Application Support/MuseTerminal/venv"
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install -r companion/requirements.txt
~~~

<code>esptool</code> is only needed for flashing; install it in step 3.

Replace YOUR_THREAD_ID with your own Muse conversation ID:

~~~bash
MUSE_VENV="$HOME/Library/Application Support/MuseTerminal/venv" ./companion/start.command --muse-url 'https://muse.ai/thread/YOUR_THREAD_ID'
~~~

If you see <code>permission denied</code>, run <code>chmod +x companion/start.command</code> once (files obtained by cloning the Git repository or downloading the source ZIP are not executable; the Release package is fine).

In an automated environment that cannot open a Terminal window, such as a sandboxed AI agent, run the bridge directly — it is equivalent:

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" companion/bridge.py \
  --muse-url 'https://muse.ai/thread/YOUR_THREAD_ID'
~~~

Wait for the speech model and a **dedicated Chrome window**. Sign in to Muse yourself in that window, open the specified conversation, and leave the composer empty. Keep the bridge's Terminal window open and the Mac awake. At the [local health page](http://127.0.0.1:18765/health), both <code>speech_ready</code> and <code>muse_ready</code> must be <code>true</code>.

## 3. Flash the firmware

Install the flashing tool first:

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install -r companion/requirements-flash.txt
~~~

If that fails — PyPI ships esptool as a source-only package, some pip versions fail to unpack it, and some regional mirrors do not carry it — retry against the official index:

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install \
  --no-cache-dir --index-url https://pypi.org/simple esptool
~~~

Flashing replaces Passport's firmware and **clears any Wi-Fi and pairing settings already on the device**, so you must provision Wi-Fi again afterwards. Make sure the connected device is your Passport. Connect it with the data cable; in a second Terminal window, enter the same source folder and run:

~~~bash
ls /dev/cu.usbmodem*
shasum -a 256 ../firmware/FoloToy-AI-Passport-full.bin
~~~

The release firmware SHA-256 is <code>ef16688d7de00035e94ddc607ec5265f4f1fb8105ae0141e3d4e1fb7b3714fc9</code>. Identify the new port that appeared when connecting Passport. If several ports appear, identify the correct one before proceeding — the Espressif one is your device:

~~~bash
system_profiler SPUSBDataType | grep -A 6 -i espressif
~~~

Replace ACTUAL_PORT below:

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m esptool --chip esp32c3 --port /dev/cu.usbmodemACTUAL_PORT write-flash 0x0 ../firmware/FoloToy-AI-Passport-full.bin
~~~

Wait for successful writing and verification, then device restart. “Set up Wi-Fi over USB before first use” is expected. Changing Wi-Fi later does not require another flash.

## 4. Configure Wi-Fi over USB

Find your Mac's local IPv4 address in System Settings → Wi-Fi → Details → TCP/IP. Keep Passport connected by USB. From source, run:

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" companion/configure.py --port /dev/cu.usbmodemACTUAL_PORT --host MAC_LAN_IP
~~~

Enter your 2.4 GHz Wi-Fi name and password when prompted; the password is hidden. After the device acknowledges saved setup and restarts, wait for “Ready.” Unplug USB and verify the battery-powered wireless connection.

Three things that commonly trip people up:

- **Start the bridge successfully at least once before provisioning.** The setup tool reads the pairing token and TLS certificate the bridge generates; provisioning fails if the bridge has never run.
- **It must be 2.4 GHz.** If your router exposes 2.4 GHz and 5 GHz under **different names**, pick the 2.4 GHz one. Choosing the 5 GHz SSID still prints “setup saved”, but the device then never connects.
- The <code>--host</code> value is written into the device configuration and used permanently. Reserve that address for your Mac in the router; **if the Mac's IP changes the device goes offline and you must provision again over USB**. Changing Wi-Fi also just means re-running this step — no reflash and no new Muse sign-in.

## 5. Speak to Muse

1. Press OK on the Ready screen to record; press OK again to stop. The limit is 30 seconds.
2. Review the recognized words. Press OK to send, or hold OK to discard and try again.
3. Read Muse's text reply. Use Up/Down to scroll, OK to speak again, or hold OK to return. Canceling locally does not undo a task already sent to Muse.

## If something goes wrong

Start at the health page, <http://127.0.0.1:18765/health>, and read the two ready fields.

**The screen asks you to check the Mac bridge and Muse page**

- <code>speech_ready=false</code>: the speech model has not finished downloading. Run <code>export HF_ENDPOINT=https://hf-mirror.com</code>, restart the bridge, and wait for the model-ready line.
- <code>muse_ready=false</code>: make sure the dedicated Chrome window is still open, signed in to Muse, sitting on the specified conversation, with an empty composer.
- Both are true but Passport still cannot connect: see below.

**The log keeps showing a browser startup failure (<code>browser_Error</code>)**

On first launch Muse may redirect through a bot-check page, which can kill the
browser mid-launch and leave a stale <code>browser-profile/SingletonLock</code> behind.
Every later launch then fails and the ready flags never come up. Current
versions of the bridge clear this stale lock automatically; if errors persist,
stop the bridge and remove it by hand:

~~~bash
rm -f "$HOME/Library/Application Support/MuseTerminal/browser-profile/SingletonLock"
~~~

Then start the bridge again.

**The screen says the setup was saved but the device never connects**

- Confirm 2.4 GHz, and that the Mac and Passport are on the same network.
- Confirm the Mac's IP has not changed; if it has, provision again over USB.

**Flashing reports <code>No module named esptool</code>**

Install esptool as shown in step 3.

**How to tell whether Passport really reached the Mac**

Run this in Terminal; you should see an ESTABLISHED connection from local port 18766 to the device:

~~~bash
lsof -nP -iTCP:18766 | grep ESTABLISHED
~~~

When delivery is uncertain, check Muse before retrying, and **never send the same message repeatedly**.

## Files and validation limits

The release package contains the merged image for 0x0 flashing, Python bridge and USB setup tool, application source, matching ELF/MAP debug artifacts, cover, and SHA256SUMS.json. The creator's original avatar video is omitted because redistribution rights were not established. The cover is generated gameplay artwork. Browser login data, Wi-Fi passwords, pairing tokens, and certificates are excluded.

Build and host tests passed, and the creator tested the voice workflow on earlier device firmware. The **exact SHA-256 merged image in this package** has not been separately reflashed and accepted on hardware. See the [technical guide](docs/muse-terminal.md).
