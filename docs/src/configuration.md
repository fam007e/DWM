# Configuration Guide

Unlike vanilla DWM where all configurations are hardcoded into `config.def.h` requiring continuous recompilation, this edition implements a live-reloading configuration system using human-readable TOML files.

---

## ⌨️ Dynamic Hotkeys (`~/.config/dwm/hotkeys.toml`)

Keybindings are declared in standard TOML. Saving the file sends `SIGUSR1` to DWM, instantly updating the active keybindings without interrupting running clients or restarting the X server.

### Structure

```toml
[vars]
terminal = "alacritty"

keys = [
  { mod="SUPER",       key="Return", desc="Terminal",          func="spawn", exec=["$terminal"] },
  { mod="SUPER",       key="r",      desc="App Launcher",      func="spawn", cmd="quickshell ipc --path \"${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/shell.qml\" call launcher toggle" },
  { mod="SUPER SHIFT", key="u",      desc="Update Center",     func="spawn", cmd="quickshell ipc --path \"${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/shell.qml\" call updatecenter toggle" },
  { mod="SUPER",       key="l",      desc="Lock Screen",       func="spawn", cmd="dwm-lock" },
  { mod="SUPER SHIFT", key="n",      desc="Toggle Warm Mode",  func="togglewarm" },
]
```

### Supported Modifier Keys
- `SUPER` (Windows key / Mod4)
- `ALT` / `MOD1`
- `CTRL`
- `SHIFT`
- Combinations: `SUPER_SHIFT`, `SUPER_CTRL`, `SUPER_ALT`, `CTRL_ALT`, etc.

---

## 🪟 Dynamic Window Rules (`~/.config/dwm/window-rules.toml`)

Window rules define client placement, tiling behaviors, and swallowing mechanics based on `WM_CLASS` and window titles.

```toml
[[rules]]
class = "dwm-calc"
isfloating = 1
isterminal = 0
noswallow = 1

[[rules]]
class = "Zenity"
isfloating = 1
noswallow = 1

[[rules]]
class = "Alacritty"
isterminal = 1
noswallow = 0

[[rules]]
class = "Brave-browser"
tag = 2
isfloating = 0
```

### Key Parameters
- `isterminal = 1`: Marks the application as a terminal capable of swallowing child GUI windows.
- `noswallow = 1`: Prevents the client window from being swallowed when spawned from a terminal.
- `isfloating = 1`: Forces the window into floating mode upon launch.
- `tag = N`: Automatically routes newly spawned windows to workspace tag `N`.

---

## ⚙️ Core Logic (`config.def.h`)

Fundamental X11 window manager parameters that dictate layout algorithms, font sizing, and borders remain in `config.def.h`:

```c
static const char *tags[] = { "", "", "", "", "󰕼", "", "", "", "" };
static const float mfact     = 0.55; /* Master area factor */
static const int nmaster     = 1;    /* Number of clients in master area */
static const int showbar     = 1;    /* Space reservation for Quickshell */
```

> [!TIP]
> After modifying `config.def.h`, run `make clean && sudo make install` to recompile the binary.
