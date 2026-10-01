[English](README.md)

# Muse 随身语音伙伴：零基础安装

把 AI Passport 变成可随身携带的 Muse 语音终端。按确定键说话，先核对识别文字，再发送到你自己的 Muse 会话；回复只显示文字，不会朗读。终端通过 Wi-Fi 连接一台保持开机的 Mac，Mac 用 Python 控制专用 Chrome 窗口，**无需 Chrome 扩展**。

有本机 AI Agent 的话，可以复制[交给 Agent 的安装指令](docs/agent-install-prompt.zh_CN.md)。你仍需亲自登录 Muse、输入 Wi-Fi 密码，并在刷机前确认。

## 准备

- AI Passport、一根能传数据的 USB 线、Mac 和 Chrome。Chrome 需为较新版本（本项目在 154 上验证通过）；桥接直接调用你已安装的系统 Chrome，**不需要**运行 <code>playwright install</code>，也不会额外下载 Chromium。
- Python 3.11 或更新版本，从 [Python 官方 macOS 下载页](https://www.python.org/downloads/macos/)安装。安装后在“终端”运行 <code>python3.13 --version</code> 检查——把 <code>python3.13</code> 换成你实际装的版本号。**macOS 自带的 <code>python3</code> 通常是 3.9，不能用于本项目**；下文命令里的解释器名请一律替换成你可用的 3.11+ 版本。
- 你自己的 Muse 账号及会话网址，格式为 <code>https://muse.ai/thread/...</code>。不要公开私人会话网址、密码或验证码。
- Mac 与终端使用同一局域网；终端只能接入 **2.4 GHz** Wi-Fi（不支持 5 GHz）。Mac 保持唤醒；网络不能隔离无线设备，防火墙需允许桥接服务的 TCP 18766 端口。

首次会下载依赖和本地语音模型。以后语音识别在 Mac 本地完成；你确认后，识别出的文字才会发给 Muse。国内网络如果长时间卡在模型下载，先执行 <code>export HF_ENDPOINT=https://hf-mirror.com</code> 再启动桥接。

## 第一步：下载

在[本仓库 Releases](https://github.com/jonwong0882-source/muse-terminal-ai-passport/releases/latest)下载 <code>muse-terminal-community-package.zip</code>，双击解压。里面的 <code>firmware</code> 是固件，<code>source</code> 是 Mac 桥接及源码。页面自带的 “Source code” ZIP 不含匹配的调试文件，安装请用交付包。

打开 Mac“终端”，输入 <code>cd </code>（末尾有空格），把解压出的 **source 文件夹**拖进终端，再按回车。下面的命令都在此目录执行。

## 第二步：安装并登录 Mac 桥接

在终端逐行运行：

~~~bash
python3.13 -m venv "$HOME/Library/Application Support/MuseTerminal/venv"
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install -r companion/requirements.txt
~~~

<code>esptool</code> 只在刷固件时需要，留到第三步再装。

把 YOUR_THREAD_ID 换成你自己的 Muse 会话 ID，运行：

~~~bash
MUSE_VENV="$HOME/Library/Application Support/MuseTerminal/venv" ./companion/start.command --muse-url 'https://muse.ai/thread/YOUR_THREAD_ID'
~~~

如果提示 <code>permission denied</code>，先执行一次 <code>chmod +x companion/start.command</code>（从 Git 仓库克隆或下载源码 ZIP 得到的文件不带可执行权限；Release 交付包里的则正常）。

在自动化环境里（例如由 AI Agent 代跑）打不开终端窗口时，可以直接运行桥接本体，效果完全相同：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" companion/bridge.py \
  --muse-url 'https://muse.ai/thread/YOUR_THREAD_ID'
~~~

等待语音模型下载和专用 Chrome 窗口打开。**在新窗口里亲自登录 Muse**，进入刚指定的会话并清空输入框。保持桥接终端窗口打开，不要让 Mac 休眠。访问[本机状态页](http://127.0.0.1:18765/health)：<code>speech_ready</code> 与 <code>muse_ready</code> 都是 <code>true</code>，Mac 端才准备好。

## 第三步：刷固件

先安装刷机工具：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install -r companion/requirements-flash.txt
~~~

若这一步失败（PyPI 上 esptool 只提供源码包，个别 pip 版本解包会报错，部分国内镜像也没有这个包），改用官方源重试：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m pip install \
  --no-cache-dir --index-url https://pypi.org/simple esptool
~~~

刷写会改写 Passport 固件，并**会清空设备上已有的 Wi-Fi 与配对设置**（刷完必须重新配网）。请先确认连接的是你的设备。用数据线连接 Passport，在另一个终端窗口进入同一 source 目录，运行：

~~~bash
ls /dev/cu.usbmodem*
shasum -a 256 ../firmware/FoloToy-AI-Passport-full.bin
~~~

交付包内固件的 SHA-256 应为 <code>ef16688d7de00035e94ddc607ec5265f4f1fb8105ae0141e3d4e1fb7b3714fc9</code>。找出本次接入后新增的端口，把下面“实际端口”替换成它；出现多个端口时先查明，不要猜——插上 Passport 后运行下面的命令，确认厂商是 Espressif 的那个才是你的设备：

~~~bash
system_profiler SPUSBDataType | grep -A 6 -i espressif
~~~

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" -m esptool --chip esp32c3 --port /dev/cu.usbmodem实际端口 write-flash 0x0 ../firmware/FoloToy-AI-Passport-full.bin
~~~

等待写入、校验成功和设备重启。首次看到“首次使用请通过 USB 配网”是正常的。换 Wi-Fi 不需要重刷固件。

## 第四步：USB 配网，随后无线使用

到 Mac“系统设置 → Wi-Fi → 详情 → TCP/IP”查找这台 Mac 的局域网 IPv4 地址。Passport 保持 USB 连接，在 source 目录运行：

~~~bash
"$HOME/Library/Application Support/MuseTerminal/venv/bin/python" companion/configure.py --port /dev/cu.usbmodem实际端口 --host Mac的局域网IP
~~~

按提示输入 2.4 GHz Wi-Fi 名称及密码；密码输入时不回显。看到“配网已保存，正在重启连接”后，等待屏幕变成“准备就绪”。拔掉 USB，用电池供电再确认无线连接。

三个容易踩的点：

- **配网前请先成功启动过一次桥接**。配网工具要读取桥接生成的配对令牌和 TLS 证书，桥接从未运行过会配网失败。
- **必须是 2.4 GHz**。如果家里 2.4 GHz 和 5 GHz 是**两个不同的名字**，一定要选 2.4 GHz 那个；选错时工具仍会提示“配网已保存”，但设备之后连不上。
- <code>--host</code> 填的 Mac 局域网 IP 会被写进设备配置长期使用。建议在路由器里把它设为 DHCP 保留地址；**Mac 的 IP 一变，设备就会失联，必须通过 USB 重新配网**。换 Wi-Fi 同样重新运行本步骤即可，不需要重刷固件，也不需要重新登录 Muse。

## 第五步：说话

1. 在“准备就绪”页按确定键开始录音，再按一次结束；最长 30 秒。
2. 检查识别文字；正确就按确定键发送，错误就长按确定键丢弃重说。
3. Muse 回复只显示文字。上、下键滚动；回复页按确定键继续说话，长按确定键返回。已发给 Muse 的任务不会因本机取消而自动撤回。

## 出问题了怎么办

先打开本机状态页 <http://127.0.0.1:18765/health>，看两个 ready 字段。

**屏幕显示“请检查 Mac 桥接与 Muse 网页”**

- <code>speech_ready=false</code>：语音模型没下好。国内网络先 <code>export HF_ENDPOINT=https://hf-mirror.com</code>，再重启桥接，等它打印模型就绪。
- <code>muse_ready=false</code>：确认专用 Chrome 窗口**没有关**、已登录 Muse、且停在指定会话页，输入框是空的。
- 两个 ready 都是 true 但终端仍连不上：见下一条。

**日志反复出现浏览器启动失败（<code>browser_Error</code>）**

首次启动时 Muse 可能先跳一次人机校验页，导致浏览器异常退出并残留
<code>browser-profile/SingletonLock</code>；此后每次启动浏览器都会失败，两个 ready 永远上不去。
新版桥接会自动清理这种残留锁；如果仍在报错，手动处理：

（先停掉桥接，然后）

~~~bash
rm -f "$HOME/Library/Application Support/MuseTerminal/browser-profile/SingletonLock"
~~~

再重新启动桥接。

**屏幕显示“配网已保存”但设备连不上**

- 确认选的是 **2.4 GHz**，且 Mac 与终端在同一局域网。
- 确认 Mac 的 IP 没变；变过就通过 USB 重新配网。

**刷固件报 <code>No module named esptool</code>**

回到第三步装 esptool。

**怎么确认终端真的连上了 Mac**

在“终端”运行，应看到一条本机 18766 端口连到终端 IP 的 ESTABLISHED 记录：

~~~bash
lsof -nP -iTCP:18766 | grep ESTABLISHED
~~~

发送状态不明时，先去 Muse 网页查看，**不要连续重发**。

## 文件与验证边界

交付包包括从 0x0 刷写的合并固件、Python 桥接与 USB 配网工具、应用源码、匹配的 ELF/MAP 调试文件、封面及 SHA256SUMS.json。用户原始公仔视频因未明确再分发许可而未包含；封面是生成的玩法示意图。浏览器登录状态、Wi-Fi 密码、配对令牌和证书也不在包内。

构建和主机测试通过，作者此前已在设备上走通语音链路；本交付包中**精确 SHA-256 的合并镜像**尚未单独重新刷机验收。更多细节见[技术说明](docs/muse-terminal.zh_CN.md)。
