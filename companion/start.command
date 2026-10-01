#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
venv="${MUSE_VENV:-$HOME/Library/Application Support/MuseTerminal/venv}"

# macOS 自带的 python3 通常是 3.9，不满足 3.11+ 的要求，所以显式挑一个合格的解释器。
pick_python() {
    local candidate
    for candidate in python3.13 python3.12 python3.11 python3; do
        if command -v "$candidate" >/dev/null 2>&1 &&
           "$candidate" -c 'import sys; raise SystemExit(0 if sys.version_info >= (3, 11) else 1)' 2>/dev/null; then
            command -v "$candidate"
            return 0
        fi
    done
    return 1
}

if [[ ! -x "$venv/bin/python" ]]; then
    if ! interpreter="$(pick_python)"; then
        cat >&2 <<'EOT'
错误：需要 Python 3.11 或更新版本，但没有找到可用的解释器。
macOS 自带的 python3 通常只有 3.9，不能用于本项目。
请从 https://www.python.org/downloads/macos/ 安装，
或运行：brew install python@3.13
安装完成后重新运行本脚本。
EOT
        exit 1
    fi
    echo "使用解释器：$interpreter（$("$interpreter" -V 2>&1)）"
    "$interpreter" -m venv "$venv"
fi

if ! "$venv/bin/python" -c "import playwright, aiohttp, serial" >/dev/null 2>&1; then
    "$venv/bin/pip" install --upgrade pip
    "$venv/bin/pip" install -r requirements.txt
fi

# 语音模型从 HuggingFace 下载。国内网络长时间卡住时，取消下面一行的注释改用镜像。
# export HF_ENDPOINT=https://hf-mirror.com
export HF_HUB_DISABLE_XET=1

exec "$venv/bin/python" bridge.py "$@"
