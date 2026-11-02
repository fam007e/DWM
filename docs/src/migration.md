# Current architecture

This page describes the current implementation; it is not a chronological change log or a security audit.

| Layer | Responsibility |
|---|---|
| `dwm.c` | X11 client management, layouts, focus, monitor work areas, EWMH state, and runtime TOML loading |
| `config/*.toml` | Shipped defaults for hotkeys, themes, and window rules |
| `config/quickshell/` | Quickshell panel, popups, controls, settings, notifications, and desktop UI |
| `scripts/` | Startup, state publishing, helper commands, installation, and system/user integration |
| `Makefile` / `install.sh` | Build/test/install workflow; the automated installer targets Arch Linux |

DWM reads user TOML from `~/.config/dwm/` when present and falls back to repository-installed defaults. User configuration files are preserved by setup/update flows. New default rule fields therefore need to be merged into existing files.

Quickshell panels reserve screen space through their panel struts. The `fillworkarea=1` DWM rule sizes designated floating windows to the monitor work area rather than using fixed screen dimensions or EWMH fullscreen. Settings and System Health use this rule.

Run `make test` for the repository's build and configuration checks. For current deployment steps, see [Install and start](install.md).
