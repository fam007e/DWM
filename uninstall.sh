#!/bin/bash
# === DWM Modern & Hardened Uninstaller ===
set -e

RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m' CYAN='\033[0;36m' NC='\033[0m'
info() { printf "${CYAN}[INFO]${NC} %s\n" "$1"; }
ok()   { printf "${GREEN}[OK]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
err()  { printf "${RED}[ERROR]${NC} %s\n" "$1"; }

confirm() {
    read -p "$1 (y/N): " response
    case "$response" in
        [yY][eE][sS]|[yY]) true ;;
        *) false ;;
    esac
}

echo ""
echo "╔═══════════════════════════════════════════╗"
echo "║       DWM Modern Upgrade Uninstaller      ║"
echo "╚═══════════════════════════════════════════╝"
echo ""

# ── 1. Remove System Binary ──────────────────────────────
info "Uninstalling DWM binary..."
if [ -f Makefile ]; then
    sudo make uninstall || {
        warn "Makefile uninstall failed, attempting manual removal."
        sudo rm -f /usr/local/bin/dwm /usr/local/share/man/man1/dwm.1
    }
fi
sudo rm -f /usr/share/xsessions/dwm.desktop
ok "Binary and desktop entry removed."

# ── 2. Remove Data Directory ─────────────────────────────
if [ -d "$HOME/.local/share/dwm" ]; then
    info "Removing data directory (~/.local/share/dwm)..."
    rm -rf "$HOME/.local/share/dwm"
    ok "Data directory removed."
fi

# ── 3. Remove Configurations ─────────────────────────────
if confirm "Remove DWM configurations (~/.config/dwm)?"; then
    rm -rf "$HOME/.config/dwm"
    ok "Configurations removed."
fi

if confirm "Remove Polybar, Rofi, and Terminal configurations?"; then
    rm -rf "$HOME/.config/polybar"
    rm -rf "$HOME/.config/rofi"
    rm -rf "$HOME/.config/alacritty"
    rm -rf "$HOME/.config/kitty"
    rm -rf "$HOME/.config/ghostty"
    ok "App configurations cleaned."
fi

# ── 4. Remove Secrets (Optional) ─────────────────────────
if confirm "Remove API secrets (~/.config/dwm_*.env)?"; then
    rm -f "$HOME/.config/dwm_weather.env" "$HOME/.config/dwm_football.env"
    ok "Secrets removed."
fi

# ── 5. Clean Caches & Logs ───────────────────────────────
info "Cleaning caches and logs..."
rm -f "$HOME/.cache/football_matches.cache" "$HOME/.cache/football_matches.log" "$HOME/.cache/weather.log" "$HOME/.cache/redshift_state"
ok "System cleaned."

echo ""
ok "Uninstallation Complete!"
warn "Note: This script did not remove installed packages (polybar, jq, etc.) or your wallpapers."
echo ""
