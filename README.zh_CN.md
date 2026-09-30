[English](README.md)

# Muse 随身语音伙伴：零基础安装

把 AI Passport 变成可随身携带的 Muse 语音终端。按确定键说话，先核对识别文字，再发送到你自己的 Muse 会话；回复只显示文字，不会朗读。终端通过 Wi-Fi 连接一台保持开机的 Mac，Mac 用 Python 控制专用 Chrome 窗口，**无需 Chrome 扩展**。

有本机 AI Agent 的话，可以复制[交给 Agent 的安装指令](docs/agent-install-prompt.zh_CN.md)。你仍需亲自登录 Muse、输入 Wi-Fi 密码，并在刷机前确认。

## 准备

- AI Passport、一根能传数据的 USB 线、Mac 和 Chrome。
- Python 3.11 或更新版本，从 [Python 官方 macOS 下载页](https://www.python.org/downloads/macos/)安装。安装后在“终端”运行 <code>python3.11 --version</code> 检查。
- 你自己的 Muse 账号及会话网址，格式为 <code>https://muse.ai/thread/...</code>。不要公开私人会话网址、密码或验证码。
- Mac 与终端使用同一局域网；终端接入 2.4 GHz Wi-Fi。Mac 保持唤醒；网络不能隔离无线设备，防火墙需允许桥接服务的 TCP 18766 端口。

首次会下载依赖和本地语音模型。以后语音识别在 Mac 本地完成；你确认后，识别出的文字才会发给 Muse。

## 第一步：下载

在[本仓库 Releases](https://github.com/jonwong0882-source/muse-terminal-ai-passport/releases/latest)下载 <code>muse-terminal-community-package.zip</code>，双击解压。里面的 <code>firmware</code> 是固件，<code>source</code> 是 Mac 桥接及源码。页面自带的 “Source code” ZIP 不含匹配的调试文件，安装请用交付包。

打开 Mac“终端”，输入 <code>cd </code>（末尾有空格），把解压出的 **source 文件夹**拖进终端，再按回车。下面的命令都在此目录执行。

## 第二步：安装并登录 Mac 桥接

在终端逐行运行：

~~~bash
python3.11 -m venv "$HOME/Library/Application Support/MuseTerminal/venv"
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install -r companion/requirements.txt esptool
~~~

把 YOUR_THREAD_ID 换成你自己的 Muse 会话 ID，运行：

~~~bash
MUSE_VENV="$HOME/Library/Application Support/MuseTerminal/venv" ./companion/start.command --muse-url 'https://muse.ai/thread/YOUR_THREAD_ID'
~~~

等待语音模型下载和专用 Chrome 窗口打开。**在新窗口里亲自登录 Muse**，进入刚指定的会话并清空输入框。保持桥接终端窗口打开，不要让 Mac 休眠。访问[本机状态页](http://127.0.0.1:18765/health)：<code>speech_ready</code> 与 <code>muse_ready</code> 都是 <code>true</code>，Mac 端才准备好。

## 第三步：刷固件

刷写会改写 Passport 固件，也可能清掉旧配网。请先确认连接的是你的设备。用数据线连接 Passport，在另一个终端窗口进入同一 source 目录，运行：

~~~bash
ls /dev/cu.usbmodem*
shasum -a 256 ../firmware/FoloToy-AI-Passport-full.bin
~~~

交付包内固件的 SHA-256 应为 <code>ef16688d7de00035e94ddc607ec5265f4f1fb8105ae0141e3d4e1fb7b3714fc9</code>。找出本次接入后新增的端口，把下面“实际端口”替换成它；出现多个端口时先查明，不要猜：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m esptool --chip esp32c3 --port /dev/cu.usbmodem实际端口 write-flash 0x0 ../firmware/FoloToy-AI-Passport-full.bin
~~~

等待写入、校验成功和设备重启。首次看到“首次使用请通过 USB 配网”是正常的。换 Wi-Fi 不需要重刷固件。

## 第四步：USB 配网，随后无线使用

到 Mac“系统设置 → Wi-Fi → 详情 → TCP/IP”查找这台 Mac 的局域网 IPv4 地址。Passport 保持 USB 连接，在 source 目录运行：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" companion/configure.py --port /dev/cu.usbmodem实际端口 --host Mac的局域网IP
~~~

按提示输入 2.4 GHz Wi-Fi 名称及密码；密码输入时不回显。看到“配网已保存，正在重启连接”后，等待屏幕变成“准备就绪”。拔掉 USB，用电池供电再确认无线连接。Mac 地址或 Wi-Fi 变了，就通过 USB 重新运行本步骤。

## 第五步：说话

1. 在“准备就绪”页按确定键开始录音，再按一次结束；最长 30 秒。
2. 检查识别文字；正确就按确定键发送，错误就长按确定键丢弃重说。
3. Muse 回复只显示文字。上、下键滚动；回复页按确定键继续说话，长按确定键返回。已发给 Muse 的任务不会因本机取消而自动撤回。

如果屏幕显示“请检查 Mac 桥接与 Muse 网页”，先确认本机状态页两个 ready 都为 true、专用 Chrome 仍登录该会话、Mac 没休眠、两设备在同一网络。发送状态不明时，先去 Muse 网页查看，**不要连续重发**。

## 文件与验证边界

交付包包括从 0x0 刷写的合并固件、Python 桥接与 USB 配网工具、应用源码、匹配的 ELF/MAP 调试文件、封面及 SHA256SUMS.json。用户原始公仔视频因未明确再分发许可而未包含；封面是生成的玩法示意图。浏览器登录状态、Wi-Fi 密码、配对令牌和证书也不在包内。

构建和主机测试通过，作者此前已在设备上走通语音链路；本交付包中**精确 SHA-256 的合并镜像**尚未单独重新刷机验收。更多细节见[技术说明](docs/muse-terminal.zh_CN.md)。
