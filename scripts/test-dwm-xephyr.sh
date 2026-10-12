#!/bin/bash
# scripts/test-dwm-xephyr.sh - Test DWM inside a nested Xephyr window safely
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

# 1. Check for Xephyr
if ! command -v Xephyr &>/dev/null; then
    echo "Error: Xephyr is not installed."
    echo "Install it with: sudo pacman -S xorg-server-xephyr"
    exit 1
fi

# 2. Compile local binary if missing or out-of-date
if ! make -q -C "$REPO_DIR" 2>/dev/null; then
    echo "==> Compiling local dwm binary..."
    make -C "$REPO_DIR"
fi

# 3. Find an available X11 display number
DISPLAY_NUM=""
for i in $(seq 2 20); do
    if [ ! -e "/tmp/.X11-unix/X$i" ]; then
        DISPLAY_NUM=":$i"
        break
    fi
done

if [ -z "$DISPLAY_NUM" ]; then
    echo "Error: Could not find an available X11 display between :2 and :20."
    exit 1
fi

RESOLUTION="${1:-1600x900}"
echo "==> Launching Xephyr on display $DISPLAY_NUM with resolution $RESOLUTION..."

# 4. Start Xephyr in background
Xephyr "$DISPLAY_NUM" -ac -br -noreset -screen "$RESOLUTION" &
XEPHYR_PID=$!

# Trap signals to ensure cleanup on exit
cleanup() {
    echo ""
    echo "==> Cleaning up nested session..."
    kill "$XEPHYR_PID" 2>/dev/null || true
    # Kill any quickshell instances tied to this display
    pkill -f "quickshell.*$DISPLAY_NUM" 2>/dev/null || true
    wait "$XEPHYR_PID" 2>/dev/null || true
    echo "==> Nested session terminated cleanly."
}
trap cleanup EXIT INT TERM

# Wait for Xephyr to initialize
for _ in $(seq 1 30); do
    if [ -e "/tmp/.X11-unix/X${DISPLAY_NUM#:}" ]; then
        break
    fi
    sleep 0.1
done

echo "==> Starting local DWM session on $DISPLAY_NUM..."
echo "    - Focus the Xephyr window to interact with the new desktop."
echo "    - Press Super+Shift+Q inside the window or Ctrl+C in this terminal to exit."

# 5. Launch DWM in the nested display
export DWM_DATA_DIR="$REPO_DIR"
export PATH="$REPO_DIR/scripts:$PATH"
if [ ! -f "${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/shell.qml" ]; then
    export QS_CONFIG_PATH="$REPO_DIR/config/quickshell"
fi

DISPLAY="$DISPLAY_NUM" "$REPO_DIR/dwm"
