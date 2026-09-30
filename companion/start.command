#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
venv="${MUSE_VENV:-$HOME/esp/muse-bridge-venv}"
if [[ ! -x "$venv/bin/python" ]]; then
    python3 -m venv "$venv"
    "$venv/bin/pip" install -r requirements.txt
fi
if ! "$venv/bin/python" -c "import playwright" >/dev/null 2>&1; then
    "$venv/bin/pip" install -r requirements.txt
fi
export HF_HUB_DISABLE_XET=1
exec "$venv/bin/python" bridge.py "$@"
