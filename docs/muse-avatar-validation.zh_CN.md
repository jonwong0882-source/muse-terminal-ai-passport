[English](muse-avatar-validation.md)

# Muse 公仔固件验证记录 — 2026-09-29

公仔界面使用用户提供的五秒 Muse 绒毛公仔视频。待机状态以 4 fps 播放二十帧 240×240 RGB565 动画，同时隐藏提示卡片。录音切换为深色界面与静止的 164×164 圆形头像；圆形边界预先生成，避免麦克风采集期间由 LVGL 分配绘图层。确认、回复、播放及错误状态使用白底圆形小头像；文字卡片也采用纯白底和浅边线。原视频保存在 `assets/images/muse-avatar-source.mp4`，执行 `python3 tools/generate_muse_avatar.py` 可重新生成 Flash 素材。

## 固件身份

- 分支：`codex/muse-terminal`，已有未提交修改均保留。
- ESP32-C3、8 MB Flash、ESP-IDF 5.5.3。从 `0x0` 写入的合并固件：`build/FoloToy-AI-Passport-full.bin`，5,333,744 字节，SHA-256 `dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235`。
- 位于 `0x10000` 的应用镜像：5,268,208 / 8,323,072 字节。匹配的镜像、ELF、MAP 和清单位于 `build/firmware/dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235/`。
- 完整门禁以 `IDF_COMPONENT_MANAGER=0` 使用本地缓存组件。归档校验命令：`python3 tools/archive_firmware.py verify build/firmware/dcb725bd94d8aa56b3c11624fdb84be5b0d1c1618514c08656c02030b087b235`。

## 结果

| 项目 | 结果 | 证据与边界 |
| --- | --- | --- |
| Build | PASS | 完整 `./tools/validate.sh`、合并镜像和归档校验均通过。 |
| Host tests | PASS | 协议、桥接、仓库和字库检查通过；LVGL 模拟器渲染九个状态，检查 21,490 个真实字形描述，并观察到待机动画换帧。已目视检查模拟器中的回复页，并确认画布、圆形头像边角及文字卡片背景均为 RGB(255,255,255)。 |
| Device tests | 部分 PASS | 设备分区表与归档逐字节一致；在 `/dev/cu.usbmodem21101` 仅从 `0x10000` 写入 5,268,208 字节应用，esptool 写入哈希校验通过。ROM 从 SPI Flash 启动，Mac 桥接重新建立 TCP 18766 终端连接。屏幕效果仍待用户目视确认。 |
| Unverified | 待真机验证 | 回复页的白底、圆形边缘与长文本滚动，以及 Wi-Fi/音频并行时剩余堆、麦克风稳定性和耗电。 |

本版经用户明确授权后，仅从 `0x10000` 更新应用，未写入 NVS 配网区域。Mac 桥接语音模型、网页扩展和 Muse 会话均报告就绪。串口限时观察仅捕获 ROM 启动信息，不能证明屏幕显示或麦克风音质正常；从 `0x0` 写入合并镜像仍可能清除 NVS 配网。
