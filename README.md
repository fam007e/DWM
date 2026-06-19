<div align="center">
  <img src="./dwm.png" alt="dwm-logo" width="150" height="150"/>

  # DWM - Modern & Hardened Edition
  ### An extremely ***fast***, ***small***, and ***dynamic*** window manager for X.

  [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
  [![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)
  [![Platform: Linux](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/)

</div>

---

This is a heavily customized fork of dwm designed for a modern, secure, and automated desktop experience. It moves away from the traditional "header-only" configuration, offering dynamic reloading and advanced status integration via Polybar.

## 🤝 Community Standards

- **[Code of Conduct](./CODE_OF_CONDUCT.md)** - Our pledge for a welcoming community.
- **[Contributing Guide](./CONTRIBUTING.md)** - How to report bugs and submit PRs.
- **[Security Policy](./SECURITY.md)** - How to report vulnerabilities securely.

### 🚀 Key Features

- **Polybar Integration:** Full EWMH compliance allows for a modern, powerful status bar with workspace indicators and window titles.
- **Dynamic Hotkeys (TOML):** Change your keybindings in `~/.config/dwm/hotkeys.toml` and they apply **instantly** without recompilation.
- **Live Theme Engine:** Switch between Nord, Dracula, and other themes dynamically. All components (DWM, Alacritty, Rofi, Polybar) sync colors automatically.
- **Hardened Security:** API keys and sensitive location data are isolated into restricted `.env` files with strict 600 permissions.
- **Fully Portable:** The build system uses relative discovery. You can clone and run this repository from any directory.
- **Automated Football Ticker:** A custom hybrid module that combines official API data with a multithreaded scraper to show upcoming matches for top international teams.
- **Window Swallowing:** Terminals automatically "swallow" GUI windows launched from them to save screen real estate.

---

## 📚 Documentation

For detailed information on how to use and customize this build, please refer to the following guides:

- **[Installation Guide](./docs/INSTALL.md)** - Dependencies and hardened deployment.
- **[Configuration Guide](./docs/CONFIGURATION.md)** - Dynamic hotkeys and TOML settings.
- **[Key Features](./docs/FEATURES.md)** - Football ticker, Weather, and Warm Mode.
- **[Security Model](./docs/SECURITY.md)** - Secret isolation and permissions.
- **[Theming Engine](./docs/THEMING.md)** - Centralized color synchronization.

---

## 📋 Installation

### 1. Dependencies
This build is optimized for Arch Linux. Ensure you have the required tools:

```bash
sudo pacman -S --needed base-devel libx11 libxft libxinerama imlib2 libxcb xcb-util \
    xorg-server xorg-xinit \
    polybar rofi picom feh flameshot python jq bc curl playerctl blueman
```

### 2. Build & Install
```bash
git clone <your-repo-url>
cd DWM
make clean && sudo make install
./install.sh
```

### 3. Testing
To verify the build stability and script integrity:
```bash
make test
```

### 4. Uninstall
To remove the build and its configurations:
```bash
./uninstall.sh
```

---

## ⌨️ Keybindings

The default modifier key is **SUPER** (Windows Key).

| Keybind | Action |
|---------|--------|
| <kbd>SUPER</kbd> + <kbd>Return</kbd> | Launch Alacritty |
| <kbd>SUPER</kbd> + <kbd>R</kbd> | App Launcher (Rofi) |
| <kbd>SUPER</kbd> + <kbd>Q</kbd> | Close focused window |
| <kbd>SUPER</kbd> + <kbd>/</kbd> | **Keybindings Help (Live)** |
| <kbd>SUPER</kbd> + <kbd>SHIFT</kbd> + <kbd>N</kbd> | **Toggle Warm Mode (Native C)** |
| <kbd>SUPER</kbd> + <kbd>SHIFT</kbd> + <kbd>F</kbd> | Football Match Selector |
| <kbd>SUPER</kbd> + <kbd>U</kbd> / <kbd>W</kbd> | Bluetooth / WiFi Menu |
| <kbd>SUPER</kbd> + <kbd>1-9</kbd> | Switch to tag (workspace) |
| <kbd>SUPER</kbd> + <kbd>Shift</kbd> + <kbd>P</kbd> | Screenshot (Area) |
| <kbd>SUPER</kbd> + <kbd>Shift</kbd> + <kbd>Q</kbd> | Quit DWM |

*For a full list of shortcuts, press <kbd>SUPER</kbd> + <kbd>/</kbd> or edit `~/.config/dwm/hotkeys.toml`.*

---

## 🔧 Configuration

This build uses a tiered configuration system:

1.  **Hotkeys & Layouts:** Edit `~/.config/dwm/hotkeys.toml`.
2.  **Themes & Colors:** Edit `~/.config/dwm/themes.toml`.
3.  **Window Rules:** Edit `~/.config/dwm/window-rules.toml` (Swallowing, Floating, Tags).
4.  **Appearance & Logic:** Edit `config.def.h` and run `make && sudo make install`.
5.  **Secrets:** Configure your API keys in `~/.config/dwm/secrets/weather.env` and `~/.config/dwm/secrets/football.env`.

---

## 📁 Project Structure

| Path | Purpose |
|------|---------|
| `dwm.c` | Core window manager source |
| `config/` | Root for Polybar, Rofi, and Terminal configs |
| `scripts/` | System backend (Weather, Football, Wallpapers) |
| `install.sh` | Hardened deployment script |
| `Makefile` | Portable build system |

---

## 🛡️ Security Note
This build enforces strict file permissions (`umask 077`). Cached data and logs in `~/.cache` are owner-readable only to prevent data leakage between users on the same system.
