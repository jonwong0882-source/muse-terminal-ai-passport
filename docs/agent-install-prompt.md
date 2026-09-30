[简体中文](agent-install-prompt.zh_CN.md)

# Installation prompt for a local AI agent

Copy the block below to a trusted local agent with Mac Terminal and download access. **Do not paste Muse passwords, Wi-Fi passwords, verification codes, or tokens into chat.**

~~~text
Help me install Muse Terminal from https://github.com/jonwong0882-source/muse-terminal-ai-passport on this Mac. I am a beginner. Give me one action at a time when I must participate, and explain what success looks like.

Read README.md and docs/muse-terminal.md. Download the latest Release's muse-terminal-community-package.zip. Check ZIP integrity and every file against SHA256SUMS.json. Confirm firmware/FoloToy-AI-Passport-full.bin is a merged image for flashing at 0x0. The image described here has SHA-256 ef16688d7de00035e94ddc607ec5265f4f1fb8105ae0141e3d4e1fb7b3714fc9. If the latest Release differs, explain the version difference before flashing.

Check for Python 3.11+, Chrome, 2.4 GHz Wi-Fi, and a USB data cable. Install missing software from official sources. Create a project-specific virtual environment and install source/companion/requirements.txt plus esptool. Ask me for my own Muse conversation URL and let me sign in myself in the dedicated Chrome window. Never request, view, or record passwords or verification codes. Start the Python bridge and wait until speech_ready and muse_ready are both true on its local /health endpoint.

Identify the actual Passport /dev/cu.usbmodem* port; ask me to select it if there are multiple candidates. Before flashing, show me the device, port, image path, and SHA-256, then explicitly ask for my approval. Do not flash or erase the whole chip without approval. Once approved, write the merged image at 0x0 and verify success and device startup.

Find this Mac's current LAN IPv4 address and confirm the Mac and Passport can reach each other on the same local network. Run source/companion/configure.py over USB. Let me enter my 2.4 GHz Wi-Fi name and password in its hidden terminal prompt; never store them in chat, scripts, logs, or Git. Confirm the saved-setup message, restart, and Ready screen. Unplug USB and verify wireless operation. If a firewall blocks TCP 18766, explain the narrowest allow rule first.

Finally, ask me to record a harmless test phrase, review the transcript, and confirm sending it. Verify a Muse text reply reaches Passport. Report the exact firmware hash, bridge state, wireless connection, recognition, and Muse round trip separately, marking anything unverified. Do not auto-retry uncertain sends or approve purchases, emails, or other Muse actions for me. Offer user-level LaunchAgent autostart only if I want it, and explain how to disable it.
~~~
