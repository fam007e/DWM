# Desktop features

## DWM and monitors

DWM provides tiling and floating windows, keyboard-driven focus and tags, client rules, and window swallowing. Its TOML window rules can make selected windows fill the monitor's **work area**: DWM computes that area from the active monitor and reserved panel struts. Settings and System Health use this behavior without advertising EWMH fullscreen, so their geometry adapts to each output and leaves the panel unobscured.

## Quickshell panel and controls

The Quickshell panel shows workspaces, the focused window, running applications, a system tray, clock/calendar, and system status. Controls include audio output, microphone mute, brightness, battery, network, Bluetooth, and power. Hardware brightness shortcuts use `brightnessctl`; microphone/audio shortcuts use PipeWire tools with PulseAudio-compatible fallbacks where configured.

The shell also includes an application launcher, command menu, Control Center, Settings, System Health, weather, update center, and SysPill system-resource view. SysPill's popup follows its selected RAM, CPU, or combined mode and lists up to 12 processes ranked by memory in RAM mode and CPU otherwise; the list shows only the selected metric or both in combined mode. Some actions depend on optional system tools or services.

## Notifications

Quickshell hosts the desktop notification service and provides styled popups and notification history. Do not run a second notification daemon that also owns `org.freedesktop.Notifications`. Default shortcuts dismiss the latest notification (`Alt+,`) and toggle Do Not Disturb (`Alt+Shift+,`).

## Weather, calendar, themes

- Weather can use OpenWeatherMap credentials from `~/.config/dwm/secrets/weather.env`; the helper also supports a `wttr.in` fallback.
- Calendar agenda integration uses `khal` when installed.
- Wallpaper selection can generate a theme palette and apply it to supported desktop components.

See [configuration](configuration.md) for hotkeys and window rules, and [theming](theming.md) for theme setup.
