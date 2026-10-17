# Installation & Setup

This guide details the complete deployment process for **Arch Linux** and derivatives.

---

## 📦 Core Dependencies

The deployment script `install.sh` manages package verification and directory initialization. Ensure `pacman` has access to the standard and multilib repositories.

### Package Manifest

| Domain | Packages | Purpose |
|---|---|---|
| **Build Core** | `base-devel`, `libx11`, `libxft`, `libxinerama`, `imlib2`, `libxcb`, `xcb-util`, `freetype2`, `pkgconf` | Compiling the DWM C binary with X11/XCB extensions |
| **X11 Display** | `xorg-server`, `xorg-xinit`, `xorg-xrandr`, `xorg-xset`, `xorg-xsetroot`, `xdotool`, `wmctrl` | X Server, screen layout control, and client manipulation |
| **Desktop Shell** | `quickshell` (AUR/prebuilt), `qt6-declarative`, `qt6-svg` | Next-gen desktop panel, workspaces, toolbars, and popups |
| **System Utilities** | `brightnessctl`, `playerctl`, `feh`, `picom`, `flameshot`, `zenity`, `libqalculate`, `slock` | Hardware controls, compositor, screenshots, dialogs, calculator, screen locker |
| **Calendar & Agenda** | `khal` *(optional)* | Calendar event synchronization, agenda queries, and interactive management |
| **Parsing & Scripts** | `python`, `python-pillow`, `jq`, `bc`, `curl` | Wallpaper color quantization, JSON parsing, weather fetching |
| **Audio & Network** | `wireplumber` / `pipewire-pulse`, `networkmanager`, `bluez`, `bluez-utils` | Audio mixer and connectivity daemons |

To install the official repo packages on Arch Linux:

```bash
sudo pacman -S --needed base-devel libx11 libxft libxinerama imlib2 libxcb xcb-util \
    xorg-server xorg-xinit xorg-xrandr xorg-xset xorg-xsetroot \
    picom feh flameshot python python-pillow jq bc curl playerctl \
    brightnessctl xdotool wmctrl zenity libqalculate khal slock
```

> [!NOTE]
> If you do not yet have Quickshell installed, install it from the AUR:
> ```bash
> yay -S quickshell
> ```

---

## 🚀 Deployment

You can deploy the desktop environment either via the automated installer or directly using Make targets.

### Method A: Automated Installer (Recommended)

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/fam007e/DWM.git
   cd DWM
   ```

2. **Run the Installer:**
   ```bash
   ./install.sh
   ```
   The `install.sh` script executes the following:
   - Validates all system and optional dependencies.
   - Compiles and installs the C binary with root helpers (`sudo make install-system`).
   - Records the system installation receipt (`/usr/local/share/dwm/desktop-install.json`).
   - Synchronizes user configurations and records state receipts (`record-user`).
   - Mirrored deployment of `config/quickshell` into `~/.config/quickshell`.
   - Seeds initial TOML configurations (`hotkeys.toml`, `themes.toml`, `window-rules.toml`) in `~/.config/dwm/` (preserves existing configs without overwriting).
   - Generates `~/.config/environment.d/dwm.conf` for session variable propagation.

### Method B: Manual / Build-System Deployment

```bash
cd DWM
make clean

# Compile binary, copy scripts, libexec helpers, and record system receipt
sudo make install-system

# Record user profile state and sync desktop config
python3 scripts/dwm-desktop-update record-user .
```

---

## 🖥️ Session Launch

### Using `startx` / TTY
Ensure your `~/.xinitrc` ends with:
```bash
exec dwm
```

If you launch X11 automatically on login to Virtual Terminal 1, place this in `~/.zprofile` or `~/.bash_profile`:
```bash
if [[ -z $DISPLAY && $XDG_VTNR -eq 1 ]]; then
    exec startx
fi
```

### Using Display Managers (SDDM / LightDM)
A desktop entry `dwm.desktop` is installed to `/usr/share/xsessions/dwm.desktop`, allowing selection directly from the login greeter.
