# Installation Guide

This build is designed for **Arch Linux** and uses a hardened, portable installation procedure.

## 1. Prerequisites
Ensure your system is up to date and you have a working Xorg environment.

## 2. Core Dependencies
The `install.sh` script will attempt to install these automatically, but you should ensure `pacman` is available:

| Component | Packages |
|-----------|----------|
| **Build** | `base-devel`, `libx11`, `libxft`, `libxinerama`, `imlib2`, `libxcb`, `freetype2`, `xorg-server`, `xorg-xinit` |
| **Status Bar** | `polybar`, `jq`, `bc`, `curl` |
| **Utilities** | `rofi`, `picom`, `feh`, `flameshot`, `blueman`, `playerctl` |
| **Logic** | `python` (for Football Scraper) |

## 3. Deployment
Run the following commands in the root of the repository:

```bash
# Compile and install the DWM binary
make clean && sudo make install

# Deploy configurations and system scripts
./install.sh
```

## 4. Manual Login Setup
Since this build respects your manual TTY workflow, ensure your `~/.zprofile` and `~/.xinitrc` are configured:

**~/.zprofile:**
```bash
if [[ -z $DISPLAY && $XDG_VTNR -eq 1 ]]; then
  exec startx
fi
```

**~/.xinitrc:**
```bash
#!/bin/bash
exec dwm
```

## 5. Post-Install Secrets
You must configure your API keys for the status bar to function:
- Create `~/.config/dwm/secrets/weather.env` for Weather data.
- Create `~/.config/dwm/secrets/football.env` for the Football ticker.
- See [SECURITY.md](./SECURITY.md) for the exact format.
