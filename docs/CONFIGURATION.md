# Configuration Guide

This DWM build uses a hybrid configuration system that combines traditional C headers with modern, live-reloading TOML files.

## 1. Hotkeys (`~/.config/dwm/hotkeys.toml`)
Your keybindings are defined in a human-readable TOML format. 
- **Live Reload:** Saving this file triggers an instant update in DWM. No restart required.
- **Variables:** Use the `[vars]` section to define your terminal (`alacritty`) or common commands.
- **Format:**
  ```toml
  { mod="SUPER", key="Return", desc="Terminal", func="spawn", exec=["$terminal"] }
  ```

## 2. Themes (`~/.config/dwm/themes.toml`)
Control the entire desktop's aesthetic from one file.
- Changes here sync **DWM**, **Polybar**, **Alacritty**, and **Rofi** simultaneously.
- To switch themes, change the `theme` value under the `[active]` section.

## 3. Window Rules (`~/.config/dwm/window-rules.toml`)
Define how specific applications behave when launched.
- **Window Swallowing:** Set `isterminal = 1` for your terminal emulators.
- **Floating:** Force applications like `pavucontrol` or `blueman` to float.
- **Tag Assignment:** Automatically send apps like Discord or Steam to specific tags.
- **Live Reload:** Just like hotkeys, saving this file applies the new rules immediately.

## 4. Core Logic (config.def.h)
While most settings are dynamic, core architectural settings still live in the C header:
- Layout definitions (Tile, Float, Monocle).
- Border pixel widths (if not overridden by theme).
- **Note:** Changes here require `sudo make install`.

## 5. Path Discovery (Portability)

This build is **directory-agnostic**. It uses the following logic to find its backend:
1. It checks the directory where it was compiled (`DWM_PATH`).
2. It falls back to `~/.local/share/dwm/scripts`.
This allows you to clone the repo anywhere without breaking script-based keybindings.
