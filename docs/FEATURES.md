# Key Features

This build includes several custom features that go beyond standard DWM patches.

## 1. Automated Football Ticker
A hybrid "Ticker" module for Polybar.
- **Data Source:** Combines official API data with a multithreaded Python scraper for `footybite.do`.
- **Filtering:** Strictly follows your personal list of Top 20 International teams and Top 38 UEFA clubs.
- **Time Sync:** Automatically converts match times from "UK Time" to your local system time.
- **Animation:** Slides smoothly across the center of your Polybar.

## 2. Hardened Weather Logic
A custom Bash implementation (`OpWeatherinfo`) that replaces the Legacy default.
- **Secure:** Loads API keys and coordinates from an isolated environment file.
- **Detailed:** Provides Temperature, "Feels Like," and Humidity.
- **Efficient:** Fetches data every 20 minutes to respect API rate limits.

## 3. Native C Warm Toggle
Unlike other builds that rely on external scripts like `redshift`, this build has the "Warm Mode" logic **physically integrated into the C binary**.
- **Accuracy:** Captures and compensates for current brightness levels during the color shift.
- **Performance:** Instantaneous toggling with zero process overhead.
- **Keybinding:** Triggered with `SUPER + SHIFT + N`.

## 4. Dynamic Window Rules
This build moves window rules out of the C header and into `window-rules.toml`.
- **Swallowing:** Terminals automatically "swallow" GUI windows launched from them. This is now configurable per-terminal class.
- **Instant Mapping:** Move applications to tags or set them to float without recompiling.
- **Safety:** Malformed rules in the TOML will not crash the window manager; it will fall back to defaults and notify you via `notify-send`.

## 5. Live Rofi Help (`SUPER + /`)
A dynamic keybinding viewer that reads your actual `hotkeys.toml` file in real-time. 
- **Automated:** It automatically groups keys by their description.
- **Aesthetic:** Uses Pango markup for beautiful icons and formatting.
- **Searchable:** Fully integrated with Rofi's fuzzy search.

## 6. Hybrid System Utilities
- **Bluetooth/WiFi Menus:** Custom Rofi-based wrappers around `nmcli` and `bluetoothctl` for fast management.
- **Football Selector:** A GUI to toggle match tracking for specific leagues or teams.
- **Power Menu:** A hardened logout/lock/reboot/shutdown interface.

## 7. Custom Picom Animations
This build uses a specialized fork of `picom` (`fam007e/picom`) that provides modern, buttery-smooth window animations (fade, zoom, slide) that are not present in the standard upstream repository.
