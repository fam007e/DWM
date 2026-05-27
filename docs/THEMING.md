# Theming Engine

This build features a centralized theme engine that synchronizes your entire desktop environment.

## 1. How it Works
The system is driven by `scripts/theme-apply.sh`. When you save `~/.config/dwm/themes.toml` or send a `USR1` signal to DWM, the following happens:
1. The script reads the active theme from the TOML.
2. It generates `active-theme` files for **Alacritty** and **Kitty**.
3. It updates the native theme setting for **Ghostty**.
4. It updates the `@theme` line in **Rofi**.
5. It fully regenerates the color palette for **Polybar**.
6. It synchronizes **GTK** (2, 3, 4) and **Qt** (5, 6) appearance (Dark/Light mode).

## 2. Active Theme
To switch themes, edit `~/.config/dwm/themes.toml`:
```toml
[active]
theme = "nord" # Change this to "dracula", "gruvbox", etc.
```

## 3. Supported Components
- **Terminals:** Alacritty, Kitty, Ghostty (auto-reloading).
- **Status Bar:** Polybar (auto-restarting).
- **Launcher:** Rofi (instant application).
- **Desktop:** GTK settings (Gsettings/D-Bus) and Qt platform themes.

## 3. Nordic Consistency
Your build is specifically tuned for your **custom Nord palette**.
- **Background:** `#2e3440` (Matches your Alacritty config).
- **Selection:** `#3b4252` (Matches your old DWM build).
- **Rofi:** Uses your unique **Rounded Nord** theme (`roundednord.rasi`).

## 4. Polybar Customization
Polybar colors are derived directly from the theme. If you want to change the bar's appearance, adjust the `polybar_*` keys in your `themes.toml`.
