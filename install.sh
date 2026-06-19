#!/bin/bash
set -e

# === High Entropy Portable Installer ===
REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m' CYAN='\033[0;36m' NC='\033[0m'
info() { printf "${CYAN}[INFO]${NC} %s\n" "$1"; }
ok()   { printf "${GREEN}[OK]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
err()  { printf "${RED}[ERROR]${NC} %s\n" "$1"; }

# ── Arch guard first — before sourcing anything ───────────────────────────────
command -v pacman &>/dev/null || { err "This installer requires Arch Linux (pacman not found)."; exit 1; }

# Source utils after guard (may call pacman internally)
source "$REPO_DIR/scripts/dwm-utils.sh"

# ── Trap for clean failure reporting ─────────────────────────────────────────
SUDO_KEEPALIVE_PID=""
trap 'err "Install aborted at line $LINENO: $BASH_COMMAND"; [ -n "$SUDO_KEEPALIVE_PID" ] && kill "$SUDO_KEEPALIVE_PID" 2>/dev/null; exit 1' ERR

# ── Cache sudo credentials upfront; keep alive for the full install ───────────
sudo -v || { err "sudo access required. Aborting."; exit 1; }
while true; do sudo -n true; sleep 50; kill -0 "$$" || exit; done 2>/dev/null &
SUDO_KEEPALIVE_PID=$!

# Mirror src -> dst exactly, removing stale files no longer in repo.
mirror_dir() { local src="$1" dst="$2"; mkdir -p "$dst"; rsync -a --delete "$src/" "$dst/"; }
# Seed dst on first install only -- never overwrites files the user has already edited.
seed_dir()   { local src="$1" dst="$2"; mkdir -p "$dst"; rsync -a --ignore-existing "$src/" "$dst/"; }

DWM_DATA_DIR="$HOME/.local/share/dwm"
BG_DIR="$HOME/Pictures/Wallpapers"

echo ""
echo "╔═══════════════════════════════════════════╗"
echo "║    Hardened DWM Modern Upgrade Installer  ║"
echo "╚═══════════════════════════════════════════╝"
echo ""

# ── Dependencies ──────────────────────────────────────────────────────────────
info "Installing core and custom dependencies..."
install_packages base-devel libx11 libxft libxinerama imlib2 libxcb xcb-util freetype2 fontconfig \
    xorg-server xorg-xinit xorg-xrandr xorg-xset xorg-xsetroot \
    rofi rofi-emoji rofi-calc dunst feh flameshot dex mate-polkit alsa-utils git unzip xclip \
    xorg-xprop thunar gvfs tumbler thunar-archive-plugin nwg-look xdg-user-dirs \
    xdg-desktop-portal-gtk pipewire pavucontrol gnome-keyring networkmanager network-manager-applet \
    libnotify rsync python jq bc curl playerctl blueman polybar ttf-meslo-nerd noto-fonts-emoji \
    meson ninja uthash libev libconfig pcre2 pixman xorgproto xcb-util-renderutil xcb-util-image mesa \
    libepoxy cmake asciidoctor
ok "Dependencies verified."

# ── Custom Picom Build (fam007e fork) ─────────────────────────────────────────
# Check binary path first; grep on version string is fragile if format changes.
PICOM_BUILD_DIR="$HOME/.cache/dwm-build/picom"
if [ -f /usr/local/bin/picom ] && /usr/local/bin/picom --version 2>/dev/null | grep -q "fam007e"; then
    ok "Custom picom (fam007e fork) already installed."
else
    info "Building custom picom (fam007e fork) from source..."
    mkdir -p "$(dirname "$PICOM_BUILD_DIR")"
    rm -rf "$PICOM_BUILD_DIR"
    git clone --depth 1 https://github.com/fam007e/picom "$PICOM_BUILD_DIR"
    cd "$PICOM_BUILD_DIR"
    meson setup --buildtype=release build
    ninja -C build
    sudo ninja -C build install
    ok "Custom picom installed to /usr/local/bin/picom"
fi
cd "$REPO_DIR"

# ── Portable Directory Setup ──────────────────────────────────────────────────
mkdir -p "$BG_DIR"
mkdir -p "$HOME/.config/dwm/secrets"
mkdir -p "$DWM_DATA_DIR/scripts"

# ── Compile & Install Binary ──────────────────────────────────────────────────
info "Compiling portable DWM binary..."
cd "$REPO_DIR"
make clean && make
sudo make install
ok "DWM binary installed to /usr/local/bin/dwm"

# ── Configuration Deployment ──────────────────────────────────────────────────
info "Deploying configurations..."

# Rofi & Terminals (seed — preserve user edits)
seed_dir "$REPO_DIR/config/rofi"      "$HOME/.config/rofi"
seed_dir "$REPO_DIR/config/alacritty" "$HOME/.config/alacritty"
seed_dir "$REPO_DIR/config/kitty"     "$HOME/.config/kitty"
seed_dir "$REPO_DIR/config/ghostty"   "$HOME/.config/ghostty"

# Polybar:
#   scripts/ → mirror (must stay in sync with repo, user should not edit these)
#   themes/  → seed  (user may customise themes)
#   fonts/   → seed  (no reason to delete user fonts)
#   top-level files (launch.sh, fonts.ini) → seed (preserve user edits)
warn "Mirroring polybar/scripts — local changes there will be overwritten."
mirror_dir "$REPO_DIR/config/polybar/scripts" "$HOME/.config/polybar/scripts"
seed_dir   "$REPO_DIR/config/polybar/themes"  "$HOME/.config/polybar/themes"
seed_dir   "$REPO_DIR/config/polybar/fonts"   "$HOME/.config/polybar/fonts"
rsync -a --ignore-existing \
    --exclude=scripts --exclude=themes --exclude=fonts --exclude=screenshots \
    "$REPO_DIR/config/polybar/" "$HOME/.config/polybar/"
chmod +x "$HOME/.config/polybar/launch.sh"
# Only make actual scripts executable, not data files
find "$HOME/.config/polybar/scripts" -type f \( -name "*.sh" -o -name "*.py" \) -exec chmod +x {} +

# TOML config (seed with explicit warning on skip)
for toml in hotkeys.toml themes.toml window-rules.toml; do
    if [ ! -f "$HOME/.config/dwm/$toml" ]; then
        cp "$REPO_DIR/config/$toml" "$HOME/.config/dwm/$toml"
        ok "Deployed $toml"
    else
        warn "$toml already exists — skipped. Check $REPO_DIR/config/$toml for upstream changes."
    fi
done

# .xinitrc (seed with explicit warning on skip)
if [ ! -f "$HOME/.xinitrc" ]; then
    cp "$REPO_DIR/scripts/.xinitrc" "$HOME/.xinitrc"
    ok "Deployed .xinitrc"
else
    warn ".xinitrc already exists — skipped. Manually check $REPO_DIR/scripts/.xinitrc for changes."
fi

# ── Session Environment (SDDM-compatible) ─────────────────────────────────────
# systemd reads ~/.config/environment.d/*.conf and exports vars into the user session.
# This ensures DWM_DATA_DIR and DWM_POLYBAR_SCRIPTS are available to polybar
# regardless of whether you use startx or SDDM.
info "Writing session environment vars..."
mkdir -p "$HOME/.config/environment.d"
cat > "$HOME/.config/environment.d/dwm.conf" <<EOF
DWM_DATA_DIR=$DWM_DATA_DIR
DWM_POLYBAR_SCRIPTS=$HOME/.config/polybar/scripts
EOF
ok "Session env written to ~/.config/environment.d/dwm.conf"

# ── Scripts & Backend Deployment ──────────────────────────────────────────────
info "Deploying system scripts to $DWM_DATA_DIR/scripts..."
mirror_dir "$REPO_DIR/scripts" "$DWM_DATA_DIR/scripts"
# Scripts with extensions
find "$DWM_DATA_DIR/scripts" -type f \( -name "*.sh" -o -name "*.py" \) -exec chmod +x {} +
# Scripts without extensions (bluetoothmenu, wifimenu, wallpapersSS, etc.)
find "$DWM_DATA_DIR/scripts" -type f ! -name "*.*" -exec chmod +x {} +

# Mirror ghostty themes into data dir
mirror_dir "$REPO_DIR/config/ghostty/themes" "$DWM_DATA_DIR/ghostty/themes"

# ── Final Security & Secret Verification ──────────────────────────────────────
info "Running security check..."

# Migrate old secrets if they exist
[ -f "$HOME/.config/dwm_weather.env" ]  && mv "$HOME/.config/dwm_weather.env"  "$HOME/.config/dwm/secrets/weather.env"
[ -f "$HOME/.config/dwm_football.env" ] && mv "$HOME/.config/dwm_football.env" "$HOME/.config/dwm/secrets/football.env"

# Secure secrets dir
chmod 700 "$HOME/.config/dwm/secrets"
# Use find — glob fails silently when no .env files exist
find "$HOME/.config/dwm/secrets" -name "*.env" -exec chmod 600 {} +

if [ ! -f "$HOME/.config/dwm/secrets/weather.env" ]; then
    warn "MISSING: Weather secrets. Run: echo 'WEATHER_API_KEY=\"...\"' > ~/.config/dwm/secrets/weather.env"
fi

if [ ! -f "$HOME/.config/dwm/secrets/football.env" ]; then
    warn "MISSING: Football secrets. Run: echo 'FOOTBALL_API_KEY=\"...\"' > ~/.config/dwm/secrets/football.env"
fi

# Apply initial theme — surface failure instead of silencing it
if ! "$DWM_DATA_DIR/scripts/theme-apply.sh"; then
    warn "Initial theme apply failed. Run theme-apply.sh manually after login."
fi

# Stop sudo keepalive
kill "$SUDO_KEEPALIVE_PID" 2>/dev/null
SUDO_KEEPALIVE_PID=""

echo ""
ok "Installation Complete!"
info "Paths are dynamic. You can safely delete or move the clone folder now."
info "Restart DWM or re-login via SDDM to apply session environment vars."
echo ""
