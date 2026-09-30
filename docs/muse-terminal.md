[简体中文](muse-terminal.zh_CN.md)

# Muse terminal

This application connects AI Passport to an existing **Muse personal agent
account** through an always-on Mac. It uses Wi-Fi for daily operation; USB is
needed only for initial firmware installation and provisioning. It does not
substitute a new Model API chatbot for the personal agent.

## Architecture and controls

The original BSP is reused without changing board pins or drivers. Startup
compiles `main/muse_main.c`, not the baseline demo menu. The portrait UI uses
the user-supplied Muse plush character as its main view. It sways and blinks at
idle, while its instruction card is hidden for a cleaner home screen. Recording
uses a dark focus screen with a Flash-backed circular portrait, timer, and
progress bar; the portrait stays still to protect the audio path. Review,
reply and error states show a compact circular portrait above a scrollable
text card. Status, recording progress, battery and button hints remain visible.

| State | OK | UP / DOWN | Long OK |
| --- | --- | --- | --- |
| Ready | Start recording | No action | Return to ready |
| Recording | Finish recording | No action | Discard recording |
| Review | Send the displayed transcript | Scroll | Discard |
| Waiting | No action | No action | Stop waiting |
| Reply | Start another recording | Scroll | Return to ready |

Recording stops after 30 seconds. Cancelling after sending does **not** cancel an
already-running Muse task; manage that task in Muse. A connection failure or
ambiguous send never automatically resubmits a command. Purchases, sending mail
and other Muse approval cards remain in the original Muse interface.

The diagnostic firmware identifies failures that occur while recording. Its error card shows whether the microphone read failed, the connection to the Mac dropped, or the audio send queue blocked, together with the recording second. Report the exact card text when investigating an interruption. This version identifies the failing stage; it does not yet repair the underlying fault.

The screen dims after 60 seconds without button input. Continuous Wi-Fi listening
keeps the terminal available; this version does not deep-sleep. Battery life and
network/audio heap use still require physical measurements.

## Mac bridge

Use Python 3.11+ on macOS. The Python bridge launches a dedicated Chrome window
through Playwright; no Chrome extension is required. Install the pinned
dependencies in the private bridge environment:

```bash
python3.11 -m venv "$HOME/esp/muse-bridge-venv"
"$HOME/esp/muse-bridge-venv/bin/pip" install -r companion/requirements.txt
./companion/start.command --muse-url https://muse.ai/thread/YOUR_THREAD_ID
```

The first start downloads the multilingual faster-whisper `base` model.
Recognition runs locally without a cloud ASR key. `--model small` selects a
larger model after downloading it. Raw recordings are held in bounded memory,
not saved as an archive. Muse replies are displayed as text only.

The bridge opens a separate Chrome profile in
`~/Library/Application Support/MuseTerminal/browser-profile/`. Sign in to Muse
in that window once, then open the exact dedicated conversation configured with
`--muse-url`. Keep its input box empty. The browser profile, pairing secrets,
and TLS identity remain outside the repository. The bridge never reads the
ordinary Chrome profile, uses private Muse APIs, or approves payments, mail, or
other confirmation cards. It clicks Send once per confirmed command; an
ambiguous outcome is not retried. Website DOM changes may require updating
`companion/muse_web.py`.

Open `http://127.0.0.1:18765/health` for
`speech_ready`, `muse_ready`, `muse_reason`, and `job_active`. The control
service listens on loopback only. The device service listens on TCP **18766**
with TLS. Permit this port on the Mac's private LAN firewall and keep the Mac
awake and on the same network. No router port forwarding is needed. Reserve
the Mac's LAN address in the router.

The board's pairing token and TLS certificate remain under
`~/Library/Application Support/MuseTerminal/` and are verified as before.
The browser profile can contain a signed-in Muse session: keep it private and
do not publish or copy it. The old `companion/extension` files are retained
for historical development only; the Python bridge does not load or poll them.

### Provision an already-flashed terminal

Discover the actual USB serial port, then run:

```bash
"$HOME/esp/muse-bridge-venv/bin/python" companion/configure.py \
  --port /dev/cu.usbmodemYOUR_DEVICE --host YOUR_MAC_LAN_IP
```

The tool prompts for a 2.4 GHz SSID and hides the password. It sends configuration
to this firmware over USB, waits for acknowledgement, and the board restarts.
It does not flash or erase the device. Credentials, pairing token and certificate
are stored in the board's `muse` NVS namespace; no credentials are embedded in the
firmware. Re-run provisioning if Wi-Fi or the Mac address changes. Standard NVS
is not protected from physical Flash extraction in this version.

## Build, validation and firmware

Activate ESP-IDF **5.5.3**, then run:

```bash
./tools/validate.sh
python3 tools/archive_firmware.py verify build/firmware/FULL_IMAGE_SHA256
```

The verified merged image is `build/FoloToy-AI-Passport-full.bin` and is intended
for offset **0x0**. Matching ELF, MAP, component images and manifest are retained
under `build/firmware/<sha256>/`. This full image may reset NVS including Wi-Fi and
pairing settings. Flashing requires explicit user approval; no flash or full-chip
erase is part of these commands.

Additional companion checks:

```bash
"$HOME/esp/muse-bridge-venv/bin/python" tests/test_muse_integration.py
"$HOME/esp/muse-bridge-venv/bin/python" -m unittest tests.test_muse_web
```

The TLS integration test simulates the board; the Python browser test uses
an isolated page fixture. Neither proves the signed-in Muse page or microphone works. The application LVGL host
renderer is under `tests/muse_sim`; use it with the pinned managed LVGL source.
It renders all eight states and checks actual glyph descriptors for the complete
[font inventory](../assets/fonts/muse-glyphs.json). Unsupported characters produce
an explicit notice; see the [font source and license](../assets/README.md).

## Physical acceptance, after flash approval

- Verify all eight Chinese UI states, long replies, unsupported glyph notices and
  mixed Latin/punctuation. Check the battery unavailable state.
- Provision Wi-Fi, disconnect USB, and confirm wireless readiness on battery.
- Record, review, cancel, send, read a text reply, and repeat after reboot. Confirm the speaker stays silent.
- Disconnect/reconnect Wi-Fi, stop the Mac service, close/sign out of the Muse
  Python-controlled browser and verify no automatic replay of an already-sent task.
- Check TLS mismatch rejection, runtime free/largest heap under audio and Wi-Fi,
  microphone levels, RF range and battery consumption.

Build/host results and the exact image identity belong in the delivery report.
A successful build, a simulated transport, or a web-client smoke test does not
establish physical device acceptance.

[Avatar firmware validation](muse-avatar-validation.md) · [Earlier diagnostic firmware validation](muse-validation.md)
