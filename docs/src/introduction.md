# DWM with Quickshell

This repository combines a customized **DWM window manager for X11** with a **Quickshell** desktop shell. The C window manager owns clients, layouts, focus, and monitor work areas; Quickshell provides the panel and desktop UI.

## How the pieces fit

- **DWM (`dwm.c`)** manages X11 windows and publishes desktop state through EWMH and project-specific X properties.
- **TOML files** configure hotkeys, themes, and window rules. DWM reloads watched files when they change.
- **Quickshell (`config/quickshell/`)** renders the panel, workspaces, tray, launchers, notifications, controls, Settings, and System Health.
- **Scripts (`scripts/`)** provide session startup, hardware/system integration, and helper commands.

The project targets Arch Linux and X11. Quickshell and other desktop utilities are runtime dependencies; package names and availability vary by distribution.

## Documentation

- [Installation and X11 session startup](install.md)
- [Features](features.md)
- [Configuration](configuration.md)
- [Theming](theming.md)
- [Updates](updates.md)
- [Security notes](security.md)
- [Current architecture](migration.md)
- [Credits](credits.md)
