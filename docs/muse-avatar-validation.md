[简体中文](muse-avatar-validation.zh_CN.md)

# Muse avatar firmware validation — 2026-09-29

The avatar UI uses the user-supplied five-second Muse plush clip. Twenty 240×240 RGB565 frames animate the idle state at 4 fps, with the instruction card hidden. Recording uses a dark screen and a static 164×164 circular portrait; its circle is pre-rendered to avoid LVGL layer allocations during microphone capture. Review, reply, playback, and error states use a white-backed circular portrait. Their text card has a pure-white fill and a subtle outline. The original clip remains under `assets/images/muse-avatar-source.mp4`; `python3 tools/generate_muse_avatar.py` regenerates the Flash asset.

## Firmware identity

- Branch: `codex/muse-terminal`, uncommitted changes preserved.
- ESP32-C3, 8 MB Flash, ESP-IDF 5.5.3. Merged image for offset `0x0`: `build/FoloToy-AI-Passport-full.bin`, 5,333,744 bytes, SHA-256 `dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235`.
- Factory app at `0x10000`: 5,268,208 / 8,323,072 bytes. Matching images, ELF, map, and manifest are in `build/firmware/dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235/`.
- The complete gate used locally cached components with `IDF_COMPONENT_MANAGER=0`. Archive verification: `python3 tools/archive_firmware.py verify build/firmware/dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235`.

## Results

| Area | Result | Evidence and limit |
| --- | --- | --- |
| Build | PASS | Complete `./tools/validate.sh`, merged image and archive verification passed. |
| Host tests | PASS | Protocol, bridge, repository, and font checks passed; the LVGL simulator rendered all nine states, verified 21,490 font glyph descriptors, and observed an idle frame change. The reply view was visually inspected; canvas, circular portrait corners, and text-card fill all measured RGB(255,255,255). |
| Device tests | PARTIAL PASS | The installed partition table matched the archive byte-for-byte. The 5,268,208-byte app was written only at `0x10000` on `/dev/cu.usbmodem21101`, and esptool verified the written hash. ROM selected SPI Flash boot; the Mac bridge regained an established TCP 18766 device connection. A person has not yet checked the screen. |
| Unverified | Pending on-device check | Actual white background, circular edge, long-text scrolling, free heap under Wi-Fi/audio load, microphone stability, and battery draw. |

After explicit approval, this revision was flashed app-only at `0x10000`; the NVS provisioning region was not written. The Mac bridge reported that the speech model, browser extension, and Muse session were ready. Bounded serial observation contained ROM boot messages only, which do not prove the display or microphone quality. A merged-image write from `0x0` could reset NVS provisioning.
