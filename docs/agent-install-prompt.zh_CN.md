[English](agent-install-prompt.md)

# 交给 AI Agent 的安装指令

把下面整段复制给你信任的本机 Agent。它需要能操作 Mac 终端和下载文件。**不要在聊天里粘贴 Muse 密码、Wi-Fi 密码、验证码或授权令牌。**

~~~text
请帮我在这台 Mac 上安装 https://github.com/jonwong0882-source/muse-terminal-ai-passport 的 Muse 终端机。我是技术小白；请一次只告诉我当前需要亲自做的一步，以及成功时会看到什么。

先读仓库 README.zh_CN.md 和 docs/muse-terminal.zh_CN.md。下载最新 Release 的 muse-terminal-community-package.zip，检查 ZIP 完整性和 SHA256SUMS.json 里每个文件的哈希；核对 firmware/FoloToy-AI-Passport-full.bin 是从 0x0 刷写的合并镜像。当前说明对应固件 SHA-256 为 ef16688d7de00035e94ddc607ec5265f4f1fb8105ae0141e3d4e1fb7b3714fc9。若最新 Release 不同，先报告版本差异，不要悄悄刷写。

如果你的运行环境有沙箱限制——不能在项目目录之外创建虚拟环境，或不能打开终端窗口、不能执行 .command 脚本——请把对应命令原样交给我手动执行并说明原因；不要静默失败，不要跳过步骤，也不要用别的方式绕过。

检查 Mac 是否有 Python 3.11+、Chrome、可用的 2.4 GHz Wi-Fi 和 USB 数据线；缺少软件时使用官方来源。创建项目专用虚拟环境，安装 source/companion/requirements.txt 和 esptool。让我提供自己的 Muse 会话网址，并在专用 Chrome 窗口亲自登录；不要索取、查看或记录密码和验证码。启动 Python 桥接，等本机 /health 的 speech_ready、muse_ready 都为 true。

发现 Passport 的真实 /dev/cu.usbmodem* 端口；若有多个候选，请我选择。刷机前展示设备、端口、固件文件和 SHA-256，并明确问我是否同意刷写；没有我确认，不烧录，也不执行全片擦除。获准后从 0x0 刷入合并固件，检查校验和启动结果。

查出 Mac 当前局域网 IPv4 地址，确认它与 Passport 可在同一局域网通信。用 USB 运行 source/companion/configure.py；让我在终端的隐藏输入提示中亲自输入 2.4 GHz Wi-Fi 名称和密码，不把密码写进聊天、脚本、日志或仓库。看到“配网已保存，正在重启连接”后确认设备进入“准备就绪”；拔掉 USB 后再验证无线连接。若防火墙阻断 TCP 18766，先解释最小范围的放行办法。

最后让我按确定键录一条无害语音，由我核对识别文字并确认发送；确认 Muse 回复回到终端文字页。分别报告固件 SHA、Mac 桥接、无线连接、语音识别和 Muse 收发的实测结果，并标出未验证项。不要自动重发状态不明的消息，不要替我批准付款、发信等操作。如我需要开机自启，再单独设置当前用户的 LaunchAgent，并告诉我如何停用。
~~~
