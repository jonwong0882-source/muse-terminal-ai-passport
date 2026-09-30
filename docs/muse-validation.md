[简体中文](muse-validation.zh_CN.md)

# Muse terminal diagnostic build — 2026-09-29

The previously flashed image has SHA-256 `f458ca9036e0a94632892a6e1885abe308205a552782ca955c45e524216d6b15`. The user confirmed its screen and Wi-Fi connection, then reported intermittent failure while speaking. That image merges microphone-read and audio-send errors into one message, so the failing operation is still unknown.

The diagnostic build separately displays microphone-read failure, Mac connection loss, or a full audio-send queue, with the recording second. It does not repair the underlying fault. The BSP and partition table are unchanged; the branch is `codex/muse-terminal` and the changes remain uncommitted.

## Artifact identity

- ESP32-C3, 8 MB Flash, ESP-IDF 5.5.3. Full image at `0x0`: `build/FoloToy-AI-Passport-full.bin`, 2,953,456 bytes, SHA-256 `d7deb57374298919f39c6abd271593ac818678b5cc84edb0f938c44ec11bb3bf`.
- Application at `0x10000`: `build/firmware/d7deb57374298919f39c6abd271593ac818678b5cc84edb0f938c44ec11bb3bf/FoloToy-AI-Passport.bin`, 2,887,920 bytes, SHA-256 `2293f15189a3bf4d04d69bafeb34e76307862fd7a8b40ab0c3726956edb6dd53`.
- Matching ELF SHA-256: `c2ff728fed7f941e92b84b668e9240f95a6d24263f7fc74818550cba6916b1de`. The verified archive binds the ELF, MAP, bootloader and partition table to the image.

## Results

| Field | Result | Evidence and limits |
| --- | --- | --- |
| Build | PASS | Complete `./tools/validate.sh` gate and archive verification; application 2,887,920 / 8,323,072 bytes. |
| Host tests | PASS | Repository, BSP, protocol, broker, TLS/HTTP and UI checks; font inventory covers the new Chinese messages. |
| Device tests | PASS for flash, network reconnect and the reported follow-up recording | Application-only write at `0x10000` on `/dev/cu.usbmodem21101` completed with esptool data verification. The ROM entered SPI Flash boot and the terminal re-established its Mac TCP connection. The user reported no connection problem in the follow-up test; the number and duration of recordings were not specified. |
| Unverified | Pending | Long-term recurrence, the original intermittent failure's cause, microphone reliability under sustained load, and audio quality. The new diagnostic error cards have not been exercised. |

The new and previously installed images have identical partition tables and application offsets. Only the application region (`0x10000` through `0x2d1fff`) was written; the observed Wi-Fi reconnection confirms the saved network configuration remained usable. The full image at `0x0` was not flashed.
