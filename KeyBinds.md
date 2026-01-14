# DWM Keybindings

This document reflects the key and mouse bindings defined in `config.def.h` (commit 9e689787868a4690be47752b7058700f6b7c8160).

## Launcher (Rofi)
- `MODKEY + r`: Launch `rofi` application launcher (drun).
- `MODKEY + Control + r`: Run `protonrestart` command.
- `MODKEY + s`: Open the search launcher (rofi dmenu prompt that opens selection in browser).
- `MODKEY + g`: Launch `rofi` chemistry periodic table.

## Emoji Launcher (Rofi)
- `Control + e`: Launch `rofi` emoji picker.

## Calculator (Rofi)
- `MODKEY + c`: Launch `rofi` calculator.
- `XF86Calculator` (hardware calculator key): Launch `rofi` calculator.

## Terminal (Alacritty)
- `MODKEY + Return`: Launch `alacritty` terminal emulator.

## Zoom
- `Control + Mod1 + t` (Control + Alt + t): Zoom the focused window.

## Open Browser (Brave)
- `MODKEY + b`: Open Brave Browser Nightly.
- `MODKEY + Shift + b`: Open Tor Browser.

## Shortcuts
- `MODKEY + a`: Launch Antigravity.
- `MODKEY + z`: Open Zed (`~/.local/bin/zed`).
- `MODKEY + slash`: Show Keybindings menu (runs the keybindings script).

## Power / Special p bindings
- `MODKEY + Mod1 + p` (MODKEY + Alt + p): Launch the power menu script.
- `MODKEY + p`: Launch the secure passwd manager (local) (`/usr/bin/securepasswd_gui`).

## Screenshots (Flameshot)
- `MODKEY + Shift + p`: Open `flameshot` GUI and save to `~/Pictures/Screenshots/`.
- `MODKEY + Control + p`: Take a full screenshot and save to `~/Pictures/Screenshots/`.

## VLC Media Player
- `MODKEY + v`: Launch `vlc`.

## Lock Screen
- `MODKEY + l`: Lock the screen using `slock`.

## File Manager (Thunar)
- `MODKEY + e`: Open `thunar` file manager.

## System Menus
- `MODKEY + w`: Launch Wi-Fi menu script.
- `MODKEY + u`: Launch Bluetooth menu script.
- `MODKEY + Control + w`: Change wallpaper (randomize from `~/Pictures/Wallpapers/`).
- `MODKEY + Shift + w`: Launch `looking-glass-client`.

## Audio Controls
- `XF86XK_AudioMute`: Mute/unmute audio (uses amixer toggle).
- `XF86XK_AudioRaiseVolume`: Increase volume by 5%.
- `XF86XK_AudioLowerVolume`: Decrease volume by 5%.

## Brightness Controls
- `XF86XK_MonBrightnessUp`: Increase screen brightness (via xrandr brightness adjust wrapper).
- `XF86XK_MonBrightnessDown`: Decrease screen brightness (via xrandr brightness adjust wrapper).

## Microphone Mute
- `XF86XK_AudioMicMute`: Toggle microphone mute (pactl set-source-mute @DEFAULT_SOURCE@ toggle).

## Airplane Mode / Wireless
- `XF86XK_WLAN`: Disable wireless radios (runs `nmcli radio all off`). Note: this command turns radios off rather than toggling state.

## Media Controls
- `XF86XK_AudioPlay`: Play/pause media (playerctl play-pause).
- `XF86XK_AudioNext`: Next track (playerctl next).
- `XF86XK_AudioPrev`: Previous track (playerctl previous).
- `XF86XK_AudioStop`: Stop playback (playerctl stop).

## Status Bar
- `MODKEY + Control + b`: Toggle the display of the status bar (togglebar).

## Window Navigation
- `MODKEY + j`: Focus the next window.
- `MODKEY + k`: Focus the previous window.
- `MODKEY + Shift + j`: Move the current window down in the stack.
- `MODKEY + Shift + k`: Move the current window up in the stack.

## Master Window
- `MODKEY + i`: Increase the number of windows in the master area.
- `MODKEY + d`: Decrease the number of windows in the master area.
- `MODKEY + h`: Decrease master area size (`setmfact -0.05`).
- `MODKEY + Control + l`: Increase master area size (`setmfact +0.05`).
- `MODKEY + Shift + h`: Increase client size factor (`setcfact +0.25`).
- `MODKEY + Shift + l`: Decrease client size factor (`setcfact -0.25`).
- `MODKEY + Shift + o`: Reset client size factor (`setcfact 0.00`).

## Layout / Tag Controls
- `MODKEY + Tab`: View the previously selected tag.
- `MODKEY + q`: Close the focused window (killclient).
- `MODKEY + t`: Set the layout to tiling.
- `MODKEY + f`: Set the layout to floating.
- `MODKEY + m`: Set the layout to monocle.
- `MODKEY + space`: Toggle between the current and the previous layout.
- `MODKEY + Shift + space`: Toggle floating mode for the focused window.

## View All Tags
- `MODKEY + 0`: View all tags.
- `MODKEY + Shift + 0`: Move the focused window to all tags.

## Monitor Focus
- `MODKEY + comma`: Focus the previous monitor.
- `MODKEY + period`: Focus the next monitor.
- `MODKEY + Shift + comma`: Move the focused window to the previous monitor.
- `MODKEY + Shift + period`: Move the focused window to the next monitor.

## Tag Key Bindings (1-9)
- `MODKEY + [1-9]`: View specific tag.
- `MODKEY + Control + [1-9]`: Toggle view of specific tag.
- `MODKEY + Shift + [1-9]`: Move the focused window to specific tag.
- `MODKEY + Control + Shift + [1-9]`: Toggle focused window to specific tag.

## Blue Light Filter
- `MODKEY + Shift + n`: Toggle warmth (blue light filter).

## Quit DWM
- `MODKEY + Shift + e`: Quit `dwm`.

## Mouse Bindings

### Layout Symbols
- `Button1` (left click on layout symbol): Set layout (tiling).
- `Button3` (right click on layout symbol): Set monocle layout.

### Window Title
- `Button2` (middle click on window title): Zoom the window.

### Status Text
- `Button2` (middle click on status text): Launch terminal (`alacritty`).

### Client Window
- `MODKEY + Button1`: Move window (drag).
- `MODKEY + Button2`: Toggle floating.
- `MODKEY + Button3`: Resize window (drag).

### Tag Bar
- `Button1`: View tag.
- `Button3`: Toggle view of tag.
- `MODKEY + Button1`: Move window to tag.
- `MODKEY + Button3`: Toggle window to tag.
