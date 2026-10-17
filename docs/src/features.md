# Desktop Shell & Features

This edition replaces conventional status bars with **Quickshell**, a high-performance, hardware-accelerated desktop shell framework built on Qt Quick and QML.

---

## 🖥️ Top Panel & Workspaces

The top panel (`DwmPanel.qml`) integrates directly with the X11 EWMH desktop protocol and custom DWM atom properties:

- **DWM Emblem / App Menu:** Displays the DWM emblem (`dwm.png`) with Nerd Font fallback. Clicking triggers the Control Center.
- **Dynamic Workspaces:** Renders 9 workspaces labeled with dedicated Nerd Font glyphs (``, ``, ``, ``, `󰕼`, ``, ``, ``, ``). Active workspaces are highlighted; occupied workspaces display dynamic state indicators.
- **Client Title Bar:** Displays the active client's title with sanitization to guard against control-character and X11 injection.
- **Running Applications Area:** Displays taskbar icons for currently active clients on the active monitor.
- **System Tray:** Full system tray support for background indicators and services.
- **Audio & Connectivity:** Volume slider with mute toggle, battery percentage, network connection status, and Bluetooth controls with live status indicators.

### 🖱️ Mouse & ⌨️ Keyboard Interactivity Across Widgets
All bar widget popups support full bidirectional interaction by both keyboard and mouse:
- **Automatic X11 Input Focus:** When any popup is opened (via hotkey or clicking a panel pill), DWM automatically grants X11 input focus to the popup. Keystrokes, navigation arrows, Enter/Space, and <kbd>Esc</kbd> are routed directly to the popup.
- **Escape to Close:** Pressing <kbd>Esc</kbd> on any popup dismisses it and cleanly returns focus to the active client.
- **Bluetooth Widget:**
  - **Status Glyphs:** Displays `󰂱` (`nf-md-bluetooth_connect`) when connected, `󰂲` (`nf-md-bluetooth_off`) when disabled/off, and `󰂯` (`nf-md-bluetooth`) when idle.
  - **Mouse:** Left-click toggles popup; Right-click immediately toggles adapter power on/off.
  - **Keyboard:** <kbd>↓</kbd> / <kbd>↑</kbd> to select devices, <kbd>Enter</kbd> / <kbd>Space</kbd> to Connect / Disconnect / Pair, <kbd>s</kbd> to scan, <kbd>p</kbd> to toggle power.
- **Network / WiFi Widget:**
  - **Mouse:** Left-click toggles popup; Right-click triggers an immediate network scan.
  - **Keyboard:** <kbd>↓</kbd> / <kbd>↑</kbd> to select network, <kbd>Enter</kbd> to connect, <kbd>w</kbd> to toggle WiFi, <kbd>r</kbd> to rescan.
- **Audio & Media Controls:**
  - **Mouse:** Mouse wheel over the volume pill adjusts volume up/down; Right-click toggles mute; Left-click opens popup.
  - **Keyboard:** <kbd>↑</kbd> / <kbd>→</kbd> volume up, <kbd>↓</kbd> / <kbd>←</kbd> volume down, <kbd>m</kbd> mute toggle, <kbd>Space</kbd> play/pause, <kbd>n</kbd> / <kbd>p</kbd> next/previous track.
- **Control Center:**
  - **Mouse:** Left-click DWM emblem to open.
  - **Keyboard:** <kbd>↓</kbd> / <kbd>↑</kbd> navigate items, <kbd>Enter</kbd> / <kbd>→</kbd> activate/enter subpage, <kbd>←</kbd> go back to overview.
- **Weather Widget:**
  - **Mouse:** Left-click opens dashboard; Right-click forces wttr.in/OpenWeatherMap refresh.
  - **Keyboard:** <kbd>r</kbd> forces refresh.
- **Calendar & Agenda:**
  - **Mouse:** Left-click opens agenda; Right-click toggles 12h/24h format.
  - **Keyboard:** <kbd>←</kbd> / <kbd>→</kbd> days, <kbd>↑</kbd> / <kbd>↓</kbd> weeks, <kbd>PageUp</kbd> / <kbd>PageDown</kbd> months, <kbd>t</kbd> today, <kbd>Enter</kbd> open interactive khal.
- **Power Menu:**
  - **Keyboard:** <kbd>↑</kbd> / <kbd>↓</kbd> navigate session options (Lock, Suspend, Restart, Shutdown, Logout), <kbd>Enter</kbd> / <kbd>Space</kbd> confirm.
- **Update Center Widget:**
  - **Indicator Pill:** Displays badge count of available system/desktop updates and recovery notifications.
  - **Mouse:** Left-click toggles Update Center popup.
  - **Keyboard:** Toggle via <kbd>SUPER</kbd> + <kbd>SHIFT</kbd> + <kbd>U</kbd>; <kbd>r</kbd> to refresh, <kbd>Esc</kbd> to dismiss.

---

## 📅 Interactive Calendar & Time Management

The panel clock pill offers a complete personal time management interface:

```
┌──────────────────────────────────────────────────┐
│  October 2026                         [ Today ]  │
│  Su  Mo  Tu  We  Th  Fr  Sa                      │
│              1   2   3   4                       │
│   5   6   7   8   9  10  11                      │
│  12 [13] 14  15  16  17  18                      │
│  19  20  21  22  23  24  25                      │
│  26  27  28  29  30  31                          │
├──────────────────────────────────────────────────┤
│  AGENDA - Tuesday, Oct 13                        │
│  ● 09:30 AM  Team Standup (Google Sync)         │
│  ● 02:00 PM  System Review & Release             │
├──────────────────────────────────────────────────┤
│  [ 12H / 24H Toggle ]      [ Open in Terminal ]  │
└──────────────────────────────────────────────────┘
```

### Clock Pill Actions
- **Left-Click:** Opens or closes the interactive Calendar popup card.
- **Right-Click:** Instantly toggles between **12-Hour (AM/PM)** and **24-Hour** display formats. The preference is automatically saved to `~/.config/dwm/clock-format.conf` and persists across sessions.

### Calendar Popup Features
- **Month Grid Navigation:** Browse past or future months with the `<` and `>` arrow controls, or click the **Today** button to immediately jump back to the current date.
- **Day Selection:** Click any day in the month grid to inspect scheduled events for that date.
- **khal Integration:** Integrated with `khal` (the standard CLI calendar tool) via `scripts/dwm-calendar`:
  - Automatically fetches and formats calendar events in JSON format.
  - Supports synced multi-calendar setups (e.g., Google Calendar, Nextcloud, local vdir).
  - Displays event start/end times and descriptions in a clean agenda list.
  - If no events are scheduled for the selected date, displays a friendly "No events scheduled" indicator.
- **Terminal Launcher:** A dedicated **Open in Terminal (khal)** button launches an interactive terminal session running `khal interactive` or `ikhal` for adding, modifying, or reviewing calendar entries.

---

## 🌦️ Expanded Weather Widget

The top bar weather pill and popup card provide a comprehensive meteorological station:

### Panel Pill
- Displays the current condition glyph and live temperature (e.g., `󰖐 27°C`) immediately adjacent to the clock pill.

### Centered Popup Card
Clicking the weather pill displays an expanded, centered weather dashboard:
- **Hero Temperature Display:** Centered large temperature readout with current condition name (e.g., `Clear Sky`) and high/low daily range indicators (`↓ 27° ↑ 31°`).
- **Location & Country:** Displays city and ISO country code (e.g., `Dhaka, BD`).
- **Three-Column Environmental Grid:**
  - **Feels Like:** Perceived temperature accounting for humidity and wind chill.
  - **Humidity:** Atmospheric relative humidity percentage.
  - **Atmospheric Pressure:** Barometric pressure in `hPa`.
- **Wind & Sky Conditions:**
  - **Wind Speed & Direction:** Current speed in `km/h` with dynamic 16-point cardinal compass direction (e.g., `ENE`, `SSW`).
  - **Wind Gusts:** Peak gust speed tracking when gusting conditions are detected.
  - **Visibility & Clouds:** Visual range in `km` and cloud cover percentage.
  - **Sun Cycle:** Exact sunrise and sunset times with day/night awareness.
- **Dual Backend Architecture:**
  - **OpenWeatherMap:** When configured via `~/.config/dwm/secrets/weather.env`, retrieves high-precision metrics including pressure, clouds, visibility, and sun cycle.
  - **wttr.in Fallback:** When no API credentials are present, automatically falls back to `wttr.in` without requiring any account setup or network configuration.
- **Auto-Refresh:** Automatically refreshes in the background every 15 minutes, with a manual refresh button available anytime.

---

## 🛠️ Quick Tools & Command Menu

Press <kbd>SUPER</kbd> + <kbd>F1</kbd> to open the Quickshell Command Menu:

- **Searchable Actions:** Search installed applications, system utilities, settings panels, and display configuration.
- **Quick Calculator:** Spawns a floating terminal running `qalc` (<kbd>SUPER</kbd> + <kbd>c</kbd>).
- **Emoji Picker:** Launches a lightweight Zenity dialog to search and copy unicode emojis directly into the clipboard (<kbd>SUPER</kbd> + <kbd>.</kbd>).
- **Web Search & Scientific Research:** Quick searches for DuckDuckGo, Chemistry tools, and NIST databases (<kbd>SUPER</kbd> + <kbd>s</kbd>).

---

## 🌙 Native C Warm Mode (Night Light)

Unlike traditional setups that require external background daemons (`redshift`, `gammastep`), Warm Mode is **compiled directly into the DWM C binary**:

- **Hardware-Level Precision:** Uses X11 XRandR gamma ramps (`XRRGetCrtcGamma` / `XRRSetCrtcGamma`) to shift screen temperature to a soothing 4500K.
- **Brightness Compensation:** Dynamically captures the current brightness and maintains luminance across toggles.
- **Instantaneous:** Zero lag, zero subprocess invocation.
- **Shortcut:** Toggle anytime with <kbd>SUPER</kbd> + <kbd>SHIFT</kbd> + <kbd>n</kbd>.
