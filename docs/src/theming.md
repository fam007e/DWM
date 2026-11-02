# Themes

DWM colors and terminal palette are defined in `~/.config/dwm/themes.toml`. The active theme is selected in `[active]`:

```toml
[active]
theme = "nord"

[appearance]
borderpx = 1
```

Available built-in palettes are listed in `config/themes.toml`, including dark and light variants plus `dynamic`. Save the user file to apply changes; DWM reloads watched configuration and runs the theme integration script.

## Wallpaper-derived palette

The wallpaper helper can extract a palette from a selected image and set the active theme to `dynamic`. The default shortcut is <kbd>Super</kbd>+<kbd>Ctrl</kbd>+<kbd>W</kbd>; its exact command is in `config/hotkeys.toml`. This requires the configured wallpaper tools and Python imaging dependencies.

Theme application spans only the components supported by the installed theme scripts and shell configuration; applications not integrated with those scripts may need separate configuration.
