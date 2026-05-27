#!/bin/bash
set -e

# === High Entropy Portable Installer ===
REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$REPO_DIR/scripts/dwm-utils.sh"

RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m' CYAN='\033[0;36m' NC='\033[0m'
info() { printf "${CYAN}[INFO]${NC} %s\n" "$1"; }
ok()   { printf "${GREEN}[OK]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
err()  { printf "${RED}[ERROR]${NC} %s\n" "$1"; }

# Enforcement of private umask for config deployment
umask 077

# Mirror src -> dst exactly, removing stale files no longer in repo.
mirror_dir() { local src="$1" dst="$2"; mkdir -p "$dst"; rsync -a --delete "$src/" "$dst/"; }
# Seed dst on first install only -- never overwrites files the user has already edited.
seed_dir()   { local src="$1" dst="$2"; mkdir -p "$dst"; rsync -a --ignore-existing "$src/" "$dst/"; }

command -v pacman &>/dev/null || { err "This installer requires Arch Linux (pacman not found)."; exit 1; }

DWM_DATA_DIR="$HOME/.local/share/dwm"
BG_DIR="$HOME/Pictures/Wallpapers"

echo ""
echo "╔═══════════════════════════════════════════╗"
echo "║    Hardened DWM Modern Upgrade Installer  ║"
echo "╚═══════════════════════════════════════════╝"
echo ""

# ── Dependencies ─────────────────────────────────────────
info "Installing core and custom dependencies..."
install_packages base-devel libx11 libxft libxinerama imlib2 libxcb xcb-util freetype2 fontconfig \
    rofi picom dunst feh flameshot dex mate-polkit alsa-utils git unzip xclip \
    xorg-xprop thunar gvfs tumbler thunar-archive-plugin nwg-look xdg-user-dirs \
    xdg-desktop-portal-gtk pipewire pavucontrol gnome-keyring networkmanager network-manager-applet \
    libnotify rsync python jq bc curl playerctl blueman polybar ttf-meslo-nerd noto-fonts-emoji
ok "Dependencies verified."

# ── Portable Directory Setup ─────────────────────────────
mkdir -p "$BG_DIR"
mkdir -p "$HOME/.config/dwm"
mkdir -p "$DWM_DATA_DIR/scripts"

# ── Compile & Install Binary ─────────────────────────────
info "Compiling portable DWM binary..."
cd "$REPO_DIR"
make clean && make
sudo make install
ok "DWM binary installed to /usr/local/bin/dwm"

# ── Configuration Deployment ─────────────────────────────
info "Deploying configurations..."

# Rofi & Terminals
mirror_dir "$REPO_DIR/config/rofi" "$HOME/.config/rofi"
mirror_dir "$REPO_DIR/config/alacritty" "$HOME/.config/alacritty"
mirror_dir "$REPO_DIR/config/kitty" "$HOME/.config/kitty"
mirror_dir "$REPO_DIR/config/ghostty" "$HOME/.config/ghostty"

# Polybar
mirror_dir "$REPO_DIR/config/polybar" "$HOME/.config/polybar"
chmod +x "$HOME/.config/polybar/launch.sh"
find "$HOME/.config/polybar/scripts" -type f -name "*.sh" -exec chmod +x {} +

# TOML Management (Preserve user edits)
cp -n "$REPO_DIR/config/hotkeys.toml" "$HOME/.config/dwm/hotkeys.toml" 2>/dev/null || true
cp -n "$REPO_DIR/config/themes.toml" "$HOME/.config/dwm/themes.toml" 2>/dev/null || true
cp -n "$REPO_DIR/config/window-rules.toml" "$HOME/.config/dwm/window-rules.toml" 2>/dev/null || true

# ── Scripts & Backend Deployment ─────────────────────────
info "Deploying system scripts to $DWM_DATA_DIR/scripts..."
cp -f "$REPO_DIR/scripts/"* "$DWM_DATA_DIR/scripts/"
chmod +x "$DWM_DATA_DIR/scripts/"*

# Mirror themes into data dir
mirror_dir "$REPO_DIR/config/ghostty/themes" "$DWM_DATA_DIR/ghostty/themes"

# ── Final Security & Secret Verification ─────────────────
info "Running security check..."

if [ ! -f "$HOME/.config/dwm_weather.env" ]; then
    warn "MISSING: Weather secrets. Run: echo 'WEATHER_API_KEY=\"...\"' > ~/.config/dwm_weather.env"
fi

if [ ! -f "$HOME/.config/dwm_football.env" ]; then
    warn "MISSING: Football secrets. Run: echo 'FOOTBALL_API_KEY=\"...\"' > ~/.config/dwm_football.env"
fi

# Apply initial theme
"$DWM_DATA_DIR/scripts/theme-apply.sh" 2>/dev/null || true

echo ""
ok "Installation Complete!"
info "Paths are dynamic. You can safely delete or move the clone folder now."
info "Restart DWM to see your new Polybar and features."
echo ""
