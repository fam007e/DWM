#!/bin/bash
set -e

# === DWM Modern & Hardened Uninstaller ===
REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m' CYAN='\033[0;36m' NC='\033[0m'
info() { printf "${CYAN}[INFO]${NC} %s\n" "$1"; }
ok()   { printf "${GREEN}[OK]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
err()  { printf "${RED}[ERROR]${NC} %s\n" "$1"; }

confirm() {
    read -r -p "$1 (y/N): " response
    case "$response" in
        [yY][eE][sS]|[yY]) return 0 ;;
        *) return 1 ;;
    esac
}

trap 'err "Uninstall aborted at line $LINENO: $BASH_COMMAND"; exit 1' ERR

echo ""
echo "╔═══════════════════════════════════════════╗"
echo "║       DWM Modern Upgrade Uninstaller      ║"
echo "╚═══════════════════════════════════════════╝"
echo ""

# ── Sudo upfront ──────────────────────────────────────────────────────────────
sudo -v || { err "sudo access required. Aborting."; exit 1; }

# ── 1. Remove System Binary ───────────────────────────────────────────────────
info "Uninstalling DWM binary..."
if [ -f "$REPO_DIR/Makefile" ]; then
    sudo make -C "$REPO_DIR" uninstall || {
        warn "Makefile uninstall failed, attempting manual removal."
        sudo rm -f /usr/local/bin/dwm /usr/local/share/man/man1/dwm.1
    }
else
    warn "Makefile not found in $REPO_DIR — removing binary manually."
    sudo rm -f /usr/local/bin/dwm /usr/local/share/man/man1/dwm.1
fi

# Desktop entry — check both common install locations
sudo rm -f /usr/local/share/xsessions/dwm.desktop /usr/share/xsessions/dwm.desktop
ok "Binary and desktop entry removed."

# ── 2. Remove Custom Picom ────────────────────────────────────────────────────
if [ -f /usr/local/bin/picom ] && /usr/local/bin/picom --version 2>/dev/null | grep -q "fam007e"; then
    if confirm "Remove custom picom (fam007e fork) from /usr/local/bin?"; then
        sudo rm -f /usr/local/bin/picom
        sudo rm -f /usr/local/share/man/man1/picom.1 \
                   /usr/local/share/man/man1/picom-trans.1 \
                   /usr/local/share/doc/picom 2>/dev/null || true
        ok "Custom picom removed."
    else
        warn "Custom picom left in place at /usr/local/bin/picom."
    fi
fi

# ── 3. Remove Data Directory ──────────────────────────────────────────────────
if [ -d "$HOME/.local/share/dwm" ]; then
    if confirm "Remove data directory (~/.local/share/dwm)?"; then
        rm -rf "$HOME/.local/share/dwm"
        ok "Data directory removed."
    else
        warn "Data directory left in place."
    fi
fi

# ── 4. Remove Configurations ──────────────────────────────────────────────────
# Ask about secrets FIRST — before removing the parent dir that contains them.
# That way the answer is still meaningful.
remove_secrets=false
if [ -d "$HOME/.config/dwm/secrets" ] || \
   [ -f "$HOME/.config/dwm_weather.env" ] || \
   [ -f "$HOME/.config/dwm_football.env" ]; then
    if confirm "Remove API secrets (~/.config/dwm/secrets/ and legacy .env files)?"; then
        remove_secrets=true
    else
        warn "Secrets will be preserved."
    fi
fi

if confirm "Remove DWM configurations (~/.config/dwm)?"; then
    if $remove_secrets; then
        rm -rf "$HOME/.config/dwm"
    else
        # Remove config dir but preserve secrets subdir
        find "$HOME/.config/dwm" -mindepth 1 -maxdepth 1 ! -name "secrets" -exec rm -rf {} +
        warn "Kept ~/.config/dwm/secrets in place."
    fi
    rm -f "$HOME/.config/dwm_weather.env" "$HOME/.config/dwm_football.env"
    ok "DWM configurations removed."
elif $remove_secrets; then
    # User wants secrets gone but kept configs
    rm -rf "$HOME/.config/dwm/secrets"
    rm -f "$HOME/.config/dwm_weather.env" "$HOME/.config/dwm_football.env"
    ok "Secrets removed."
fi

if confirm "Remove Polybar, Rofi, and terminal configurations?"; then
    rm -rf "$HOME/.config/polybar"
    rm -rf "$HOME/.config/rofi"
    rm -rf "$HOME/.config/alacritty"
    rm -rf "$HOME/.config/kitty"
    rm -rf "$HOME/.config/ghostty"
    ok "App configurations removed."
fi

# ── 5. Remove Session Environment File ───────────────────────────────────────
# Written by install.sh — exports DWM_DATA_DIR and DWM_POLYBAR_SCRIPTS into SDDM sessions.
# Leaving this behind exports stale paths into every future session.
if [ -f "$HOME/.config/environment.d/dwm.conf" ]; then
    rm -f "$HOME/.config/environment.d/dwm.conf"
    # Remove dir only if now empty
    rmdir "$HOME/.config/environment.d" 2>/dev/null || true
    ok "Session environment file removed."
fi

# ── 6. Warn About .xinitrc ───────────────────────────────────────────────────
# install.sh may have deployed this. We don't remove it automatically because
# the user may have edited it or it may predate this install.
if [ -f "$HOME/.xinitrc" ]; then
    warn ".xinitrc left in place — remove manually if it was deployed by this installer: rm ~/.xinitrc"
fi

# ── 7. Clean Caches ───────────────────────────────────────────────────────────
info "Cleaning caches and build artifacts..."
# Named cache files from known scripts
rm -f "$HOME/.cache/football_matches.cache" \
      "$HOME/.cache/football_matches.log" \
      "$HOME/.cache/weather.log" \
      "$HOME/.cache/redshift_state"
# Picom build cache
rm -rf "$HOME/.cache/dwm-build"
ok "Caches cleaned."

echo ""
ok "Uninstallation complete."
warn "Packages installed during setup (polybar, jq, pipewire, etc.) were NOT removed."
warn "Wallpapers in ~/Pictures/Wallpapers were NOT removed."
echo ""
