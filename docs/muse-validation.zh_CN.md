[English](muse-validation.md)

# Muse 终端诊断版验证记录 — 2026-09-29

此前刷入的固件 SHA-256 为 `f458ca9036e0a94632892a6e1885abe308205a552782ca955c45e524216d6b15`。用户确认屏幕和 Wi-Fi 连接正常，随后报告说话录音中间歇失败。该版把麦克风读取和音频发送错误合并为同一提示，因此尚不能确定失败操作。

诊断版分别显示麦克风读取失败、Mac 连接断开或音频发送队列已满，并标明录音第几秒。本版不修复根因。BSP 和分区表未修改；分支为 `codex/muse-terminal`，修改尚未提交。

## 固件身份

- ESP32-C3、8 MB Flash、ESP-IDF 5.5.3。从 `0x0` 写入的合并固件：`build/FoloToy-AI-Passport-full.bin`，2,953,456 字节，SHA-256 `d7deb57374298919f39c6abd271593ac818678b5cc84edb0f938c44ec11bb3bf`。
- 位于 `0x10000` 的应用镜像：`build/firmware/d7deb57374298919f39c6abd271593ac818678b5cc84edb0f938c44ec11bb3bf/FoloToy-AI-Passport.bin`，2,887,920 字节，SHA-256 `2293f15189a3bf4d04d69bafeb34e76307862fd7a8b40ab0c3726956edb6dd53`。
- 匹配 ELF SHA-256：`c2ff728fed7f941e92b84b668e9240f95a6d24263f7fc74818550cba6916b1de`。已验证归档将 ELF、MAP、bootloader 和分区表绑定到镜像。

## 测试结果

| 项目 | 结果 | 证据与边界 |
| --- | --- | --- |
| Build | PASS | 完整 `./tools/validate.sh` 门禁与归档验证；应用占 2,887,920 / 8,323,072 字节。 |
| Host tests | PASS | 仓库、BSP、协议、桥接、TLS/HTTP 与界面检查；字形清单覆盖新增中文提示。 |
| Device tests | 烧录、网络重连及本轮用户复测 PASS | 在 `/dev/cu.usbmodem21101` 从 `0x10000` 仅写入应用，esptool 回读校验通过。ROM 进入 SPI Flash 启动，终端重新建立与 Mac 的 TCP 连接。用户反馈本轮测试未再发现连接问题；录音次数和时长未说明。 |
| Unverified | 待验证 | 长期使用是否复发、此前间歇故障的根因、持续负载下的麦克风稳定性及音质。新版故障提示尚未触发。 |

新旧镜像的分区表与应用偏移相同。本次仅写入应用区域（`0x10000` 至 `0x2d1fff`）；终端重新连上 Wi-Fi，说明保存的网络配置仍可用。未刷入从 `0x0` 开始的完整镜像。
