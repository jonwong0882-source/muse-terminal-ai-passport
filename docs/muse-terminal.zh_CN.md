[English](muse-terminal.md)

# Muse 终端机

本应用通过常开 Mac 接入已有的 **Muse 个人助理账号**。日常使用 Wi-Fi 无线连接；USB 仅用于首次安装固件和配网。不会将个人助理替换为另一个 Model API 聊天机器人。

## 架构与操作

原有 BSP、引脚和驱动保持复用。启动编译 `main/muse_main.c`，不进入原 demo 菜单。竖屏界面以用户提供的 Muse 绒毛公仔为主视觉；待机时轻摆、眨眼，并隐藏提示卡片，让首页更简洁。录音切换为深色专注界面，显示圆形公仔头像、计时和进度条；头像保持静止，给音频任务留出余量。确认、回复和错误状态显示白底圆形小头像及可滚动文字卡片；状态、电量和按键提示仍可见。

| 状态 | 确定键 | 上 / 下 | 长按确定 |
| --- | --- | --- | --- |
| 就绪 | 开始录音 | 无操作 | 返回就绪 |
| 录音 | 结束录音 | 无操作 | 丢弃录音 |
| 确认文字 | 发送屏幕上的识别文字 | 滚动 | 丢弃 |
| 等待 | 无操作 | 无操作 | 停止等待 |
| 回复 | 开始下一次录音 | 滚动 | 返回就绪 |

录音最长 30 秒。发送后取消**不会撤销已经在 Muse 运行的任务**，需要在 Muse 管理。连接失败或发送结果不明确时不会自动重发。付款、发信等审批卡片仍由原 Muse 界面处理。

诊断版会在录音中断时显示具体阶段：麦克风采集失败、Mac 连接断开或音频发送队列阻塞，并显示发生在录音第几秒。排查时请记录屏幕上的完整文字。本版用于定位故障，尚未修复其根因。

60 秒没有按键操作后屏幕变暗。为保持可连接，本版持续运行 Wi-Fi，不进入深度睡眠。续航和网络/音频运行时内存仍需真机测量。

## Mac 桥接

使用 macOS 和 Python 3.11+。Python 桥接通过 Playwright 启动独立的 Chrome
窗口，**无需 Chrome 扩展**。在项目目录安装固定版本依赖：

```bash
python3.11 -m venv "$HOME/esp/muse-bridge-venv"
"$HOME/esp/muse-bridge-venv/bin/pip" install -r companion/requirements.txt
./companion/start.command --muse-url https://muse.ai/thread/YOUR_THREAD_ID
```

首次启动会下载多语言 faster-whisper `base` 模型，之后在 Mac 本地识别，
无需云端语音识别密钥。下载较大的模型后可用 `--model small`。录音只暂存
于有长度上限的内存，不建立录音档案；Muse 回复仅显示文字。

桥接会用 Playwright 打开专用 Chrome 窗口，其独立资料目录位于
`~/Library/Application Support/MuseTerminal/browser-profile/`。首次请在
该窗口自行登录 Muse，再打开 `--muse-url` 指定的独立会话，并保持输入框
为空。桥接不读取日常 Chrome 资料目录，不使用 Muse 私有接口，也不自动批准
付款、发信等确认卡片。每条已确认指令只点击一次“发送”；结果不明确时
不会自动重发。Muse 网页结构变化时可能需要更新 `companion/muse_web.py`。

访问 `http://127.0.0.1:18765/health` 可查看 `speech_ready`、
`muse_ready`、`muse_reason` 和 `job_active`。控制服务只监听本机；
设备通过 TCP **18766** 与 Mac 建立 TLS 连接。Mac 和终端须在同一网络，
Mac 应保持唤醒，私人局域网防火墙需允许该端口。无需路由器端口转发。

配对令牌、TLS 证书及浏览器资料都保存在项目外的
`~/Library/Application Support/MuseTerminal/`。专用浏览器资料可能包含
Muse 登录状态，不要公开或复制。旧的 `companion/extension` 文件仅作为
历史开发资料保留；Python 桥接不会加载或轮询它。

### 给已刷好固件的终端配网

查出真实 USB 串口后运行：

```bash
"$HOME/esp/muse-bridge-venv/bin/python" companion/configure.py \
  --port /dev/cu.usbmodemYOUR_DEVICE --host YOUR_MAC_LAN_IP
```

工具提示输入 2.4 GHz Wi-Fi 名称，并隐藏密码输入；通过 USB 发送配置，收到设备确认后，设备重启。此工具不刷机、不擦除设备。凭据、令牌和证书保存在设备 `muse` NVS 命名空间中，不嵌入固件。Wi-Fi 或 Mac 地址变化后重新配网。本版普通 NVS 不防止具备物理访问条件的 Flash 读取。

## 构建、验证和固件

激活 ESP-IDF **5.5.3** 后运行：

```bash
./tools/validate.sh
python3 tools/archive_firmware.py verify build/firmware/FULL_IMAGE_SHA256
```

验证后的合并固件位于 `build/FoloToy-AI-Passport-full.bin`，从 **0x0** 刷写。匹配的 ELF、MAP、分段镜像和清单保存在 `build/firmware/<sha256>/`。合并刷写可能重置 NVS，包括 Wi-Fi 和配对设置。必须获得明确确认后才可烧录；上述命令不刷机，也不执行全片擦除。

额外桥接检查：

```bash
"$HOME/esp/muse-bridge-venv/bin/python" tests/test_muse_integration.py
"$HOME/esp/muse-bridge-venv/bin/python" -m unittest tests.test_muse_web
```

TLS 集成测试模拟设备，Python 浏览器测试使用隔离的模拟页面；两者都不能证明真实 Muse 登录页或麦克风效果。应用 LVGL 主机渲染器位于 `tests/muse_sim`，使用锁定版本的托管 LVGL 源码。它渲染八种状态，并针对完整[字符清单](../assets/fonts/muse-glyphs.json)检查真实字形描述。清单外字符会明确提示；详见[字库来源和许可](../assets/README.zh_CN.md)。

## 获准刷机后的真机验收

- 检查八种中文界面状态、长回复、缺字提示、中文/英文/标点混排及电量不可用状态。
- 配网后拔掉 USB，用电池供电验证无线就绪。
- 录音、预览、取消、发送、阅读文字回复，再重启重复测试；确认扬声器不播放回复。
- 断开/恢复 Wi-Fi、停止 Mac 服务、关闭/退出 Python 控制的 Muse 浏览器，确认已发送任务不会自动重发。
- 检查证书不匹配拒绝连接、音频与 Wi-Fi 并行时的剩余/最大连续堆、麦克风音量、无线距离和功耗。

构建/主机测试结果和准确镜像身份在交付报告中记录。编译成功、模拟通信测试或网页收发测试都不等于真机验收。

[公仔固件验证记录](muse-avatar-validation.zh_CN.md) · [此前诊断版验证记录](muse-validation.zh_CN.md)
