# DWM Quickshell Migration: Security Audit & Technical Work Log

## 1. Executive Summary
- **Objective:** Migrate DWM status bar and desktop controls from Polybar/Rofi to Quickshell (QML).
- **Reference Implementation:** Chris Titus Tech (`dwm-titus`).
- **Target Platform:** Arch Linux (Rolling), Quickshell 0.3.1.
- **Paradigm:** Preserve TOML dynamic reload configuration (`hotkeys.toml`, `themes.toml`, `window-rules.toml`) and all user-specific keybindings, scripts, and workflows.

---

## 2. Security & Code Standards Audit

### 2.1 C Code (`dwm.c`) Vulnerabilities & Logical Flaws
| ID | Area | Finding / Risk | Remediation |
|---|---|---|---|
| **AUD-01** | Null Pointer Dereference | `updatecurrentdesktop()` in `dwm-titus` dereferences `selmon->tagset` without verifying `selmon != NULL`. Can cause SIGSEGV on display changes or startup. | Add strict `if (!selmon) return;` guard. |
| **AUD-02** | Hardcoded Layout Heuristic | `getmonlogicalindex()` uses `if (m->my >= 1080)` to pick the primary monitor. Fails on side-by-side or standard non-Titus monitor topologies. | Replace with geometric coordinate sorting (`x`, then `y`) or fallback to `m->num` / Xinerama screen index. |
| **AUD-03** | Buffer Overflow / Index Bounds | `monitor_desktops` allocated as `long[count * 5]`. If `logicalindex >= count` or `< 0`, out-of-bounds heap write occurs. | Enforce strict bounds check: `if (logicalindex < 0 \|\| logicalindex >= count) continue;` and validate integer multiplication `count * 5` against integer overflow. |
| **AUD-04** | X11 Event Storms (DoS) | User's legacy `updateclientlist()` iterated through every client calling `XChangeProperty(..., PropModeAppend)`. In an event-driven system with `xprop -root -spy`, this creates $O(N)$ root window events per change, overloading Quickshell state watchers. | Batch window IDs into a single heap array and update `_NET_CLIENT_LIST` with a single atomic `PropModeReplace`. |
| **AUD-05** | Atom Leaks & X Property Types | `_DWM_MONITOR_DESKTOPS` passed 32-bit format. On 64-bit systems, X11 expects `long` arrays when format is 32. Using `int` would cause memory corruption or truncation. | Use `long` arrays and cast to `(unsigned char *)` consistently with Xlib specifications. |
| **AUD-09** | Dock vs Floating Surface Identification | In Quickshell 0.3.x, X11 `WM_CLASS` is omitted. Relying solely on `wmclasscontains()` causes `PanelWindow` to be unmanaged while treating dialogs improperly. | In `isaltbar()`, verify `_NET_WM_WINDOW_TYPE == NetWMWindowTypeDock` with geometry aspect guard (`wa->width > wa->height`) to robustly isolate the panel dock from launcher/popup surfaces. |

### 2.2 Shell Scripts & IPC Security
| ID | Area | Finding / Risk | Remediation |
|---|---|---|---|
| **AUD-06** | Injection via Untrusted Window Properties | Window titles (`WM_NAME`) and classes (`WM_CLASS`) can contain arbitrary UTF-8 characters, newlines, tabs, and `=` signs set by untrusted client apps. Could desync or inject fields into `DwmState.qml` key-value parser. | Sanitize input in `dwm-quickshell-state`: strip carriage returns and newlines, truncate excessively long titles, strip `=` delimiters from keys. |
| **AUD-07** | Temporary File Race Condition | `dwm-status` and helper scripts using `/tmp` without strict permissions can be susceptible to symlink/race attacks. | Confine runtime files to `$XDG_RUNTIME_DIR/dwm` (mode 0700). Fallback to `mktemp -d` with strict umask 077. |
| **AUD-08** | Command Injection in Hotkeys | Shell strings in `cmd="..."` are parsed by `/bin/sh`. Untrusted dynamic input must not be concatenated without sanitization. | Use `exec = [...]` for fixed binary invocations (execvp, zero shell injection risk). Wrap dynamic IPC paths with quoted environment fallbacks. |

---

## 3. Migration Roadmap & Work Log

### Phase 1: DWM C Core Engine (`dwm.c`)
- [x] 1.1 Set `altbarclass = "quickshell"` and set bar height `bh = 0` when altbar is active.
- [x] 1.2 Define EWMH & custom root atoms (`NetDwmMonitorDesktops`, `NetDwmSelectedMonitor`, `_DWM_FULLSCREEN_MONITORS`, `DWM_TAG_UPDATE`).
- [x] 1.3 Implement hardened `updatecurrentdesktop()`, `updateselectedmonitor()`, and `updatefullscreenmonitors()`.
- [x] 1.4 Refactor `updateclientlist()` to atomic batch replacement.
- [x] 1.5 Implement robust, non-hardcoded `getmonlogicalindex()` without the `>= 1080` assumption.
- [x] 1.6 Verify compilation with zero compiler warnings under `-Wall -Wextra`.

### Phase 2: Bridge Scripts & Utilities (`scripts/`)
- [x] 2.1 Import and harden `dwm-quickshell-state` (IPC event-stream publisher with 256-char title clamp).
- [x] 2.2 Import and harden `dwm-quickshell-launcher` (desktop entry cache and launcher).
- [x] 2.3 Import Quickshell helpers (`dwm-quickshell-controls`, `dwm-quickshell-controlcenter`, `dwm-quickshell-network`, `dwm-status`).
- [x] 2.4 Verify Arch Linux tool dependencies (`wmctrl`, `xdotool`, `jq`, `brightnessctl`, `wpctl`/`pactl`, `nmcli`).

### Phase 3: Quickshell Configuration (`config/quickshell/`)
- [x] 3.1 Adapt QML tree (`panel/`, `launcher/`, `controlcenter/`, `power/`, `state/`, `notifications/`).
- [x] 3.2 Ensure styling and paths point to user's config `$HOME/.config/dwm`.
- [x] 3.3 Validate QML execution against Quickshell 0.3.1 (verified headless under Xvfb).

### Phase 4: Hotkeys & TOML Integration (`config/hotkeys.toml`)
- [x] 4.1 Preserve all existing user custom bindings (Brave, Tor, Chem Search, Football, Antigravity, etc.).
- [x] 4.2 Map Quickshell IPC triggers (`SUPER+r`, `SUPER+/`, `SUPER+F1`, `SUPER+CTRL+q`, `SUPER+ALT+p`).
- [x] 4.3 Add Quickshell window rules to `window-rules.toml`.
- [x] 4.4 Keep Rofi operational for custom search scripts.

### Phase 5: Autostart & Installer Deployment (`autostart.sh`, `install.sh`)
- [x] 5.1 Switch startup from Polybar to Quickshell daemon and dwm-status in `scripts/autostart.sh`.
- [x] 5.2 Update `install.sh` for Arch Linux package dependencies (`quickshell`, `wmctrl`, `xdotool`, `brightnessctl`) and Quickshell directory mirroring.

### Phase 6: Rofi→Quickshell Preference & Polybar Removal
- [x] 6.1 Remove `rofi`, `rofi-emoji`, `rofi-calc` from required dependencies in `install.sh` (moved to optional).
- [x] 6.2 Replace `rofi -show emoji` with `scripts/dwm-emoji-picker` (zenity + xclip + xdotool; zero rofi dependency).
- [x] 6.3 Replace `rofi -show calc` with `qalc` in floating Alacritty window (`dwm-calc` WM_CLASS).
- [x] 6.4 Replace `rofi -dmenu` web search (DDG, Chemistry, NIST) with `scripts/dwm-web-search` (zenity prompt + xdg-open).
- [x] 6.5 Replace `SUPER+u` (bluetoothmenu) and `SUPER+w` (wifimenu) rofi scripts with Quickshell IPC (`call bluetooth toggle` / `call network toggle`).
- [x] 6.6 Replace `SUPER+F1` control center with `call menu toggle` (Quickshell CommandMenu — unified search for apps, tools, settings, system).
- [x] 6.7 Add "Quick Tools" submenu to `CommandMenuCatalog.js` with Calculator, Emoji, Web Search, Chemistry, NIST entries.
- [x] 6.8 Add `toolHelperCommand()` to `Commands.qml` for dispatching Quick Tools actions.
- [x] 6.9 Extend `CommandMenuModel.runHelper()` to dispatch `target === "tool"` entries.
- [x] 6.10 Replace CTT branding in `LogoButton.qml` with Nerd Font tiling-window icon glyph.
- [x] 6.11 Add floating window rules for `dwm-calc` (calculator) and `Zenity` (search/emoji dialogs).
- [x] 6.12 Remove entire polybar deployment block from `install.sh`.
- [x] 6.13 Remove `DWM_POLYBAR_SCRIPTS` from session environment (`environment.d/dwm.conf`).
- [x] 6.14 Add `zenity`, `libqalculate` to required dependencies in `install.sh`.
- [x] 6.15 Remove `polybar` from required dependencies in `install.sh`.
- [x] 6.16 Set LogoButton in Quickshell panel to display `dwm.png` with fallback to `\uf356` (Nerd Font `nf-linux-dwm`).
- [x] 6.17 Implement dynamic wallpaper palette extraction engine (`scripts/dwm-wallpaper-palette.py`) integrated into `wallpapersSS` and `SUPER+CTRL+w`.
- [x] 6.18 Update `.gitignore` to prevent build artifacts (`dwm`, `config.h`) and temporary symlinks (`dwmbychristitustech`) from cluttering git status.
- [x] 6.19 Update `uninstall.sh` to remove `~/.config/quickshell` and cleaned polybar references.
- [x] 6.20 Completely remove Football match scraper, ticker, selector scripts, hotkeys (`SUPER+x`, `SUPER+SHIFT+f`), background loop, and documentation references.

### Phase 7: Weather Widget & Final Polybar Purge
- [x] 7.1 Rewrite `scripts/OpWeatherinfo` to support `--json` mode (structured JSON output with `temp`, `feels`, `desc`, `humidity`, `city`, `wind`, `icon`).
- [x] 7.2 Create `config/quickshell/weather/WeatherModel.qml` — auto-refresh every 15 minutes, parses JSON from `OpWeatherinfo --json`.
- [x] 7.3 Create `config/quickshell/weather/WeatherWindow.qml` — `ClickAwayPopup` card with city header, large temp & icon, 3-column stats grid (Feels Like, Humidity, Wind), and manual refresh button.
- [x] 7.4 Add interactive weather `PanelPill` to `DwmPanel.qml` positioned next to clock with hover/active state bindings and tooltip.
- [x] 7.5 Wire weather into `shell.qml` — import `qs.weather`, instantiate `WeatherModel`, pass to `DwmPanel`, instantiate `WeatherWindow`, add `IpcHandler` for IPC control.
- [x] 7.6 Add `weatherCommand()` to `Commands.qml` for consistent helper command resolution.
- [x] 7.7 Add `selectPanelPopup` close guard for weather (`if (popupId !== "weather") weatherModel.close()`).
- [x] 7.8 Delete `config/polybar/` directory entirely.
- [x] 7.9 Strip all `polybar_*` color lines from `config/themes.toml` header comments.
- [x] 7.10 Remove `PB_*` variable extraction and Polybar palette generation from `scripts/theme-apply.sh`.
- [x] 7.11 Remove Polybar references from `scripts/dwm-controlcenter`, `scripts/autostart.sh`, `scripts/check-deps.sh`.
- [x] 7.12 Replace all Polybar comments in `dwm.c` (7 occurrences), `config.h`, `config.def.h` with Quickshell references.
- [x] 7.13 Replace `spawnbar()` Polybar launch command with Quickshell IPC no-op.
- [x] 7.14 Update `README.md` — description, dependencies list (polybar → quickshell).
- [x] 7.15 Update `docs/FEATURES.md` — document Quickshell Weather Widget with dual backend.
- [x] 7.16 Update `docs/THEMING.md`, `docs/CONFIGURATION.md`, `docs/INSTALL.md` to reference Quickshell.
- [x] 7.17 Clean `uninstall.sh` — remove `~/.config/polybar` cleanup line.
- [x] 7.18 Clean `install.sh` — remove stale polybar comment.
- [x] 7.19 Verify zero polybar references remain outside historical `MIGRATION_AUDIT_LOG.md`.
- [x] 7.20 Compile DWM binary — 0 errors, 0 warnings.

### Phase 8: Expanded Weather Metrics & Hero Layout
- [x] 8.1 Expand `scripts/OpWeatherinfo` with `deg_to_compass()` and detailed OpenWeatherMap JSON metrics (`temp_min`, `temp_max`, `country`, `pressure`, `visibility`, `clouds`, `wind_dir`, `wind_deg`, `wind_gust`, `sunrise`, `sunset`).
- [x] 8.2 Add day/night condition glyph resolution based on local sunrise/sunset epochs.
- [x] 8.3 Update `config/quickshell/weather/WeatherModel.qml` to parse and publish all extended environmental properties.
- [x] 8.4 Redesign `config/quickshell/weather/WeatherWindow.qml` with centered layout, hero temperature readout, daily range indicators (`↓/↑`), 3-column stats grid, and wind gust row.

### Phase 9: Calendar & Khal Agenda Integration + 12h/24h Time Persistence
- [x] 9.1 Create `scripts/dwm-calendar` helper for structured JSON event fetching from `khal` and interactive terminal launch.
- [x] 9.2 Create `config/quickshell/calendar/CalendarModel.qml` with month grid algorithm, day selection, month navigation, today jump, and event queries.
- [x] 9.3 Create `config/quickshell/calendar/CalendarWindow.qml` popup with interactive month grid, scheduled agenda cards, format toggle, and terminal button.
- [x] 9.4 Update `config/quickshell/core/ClockModel.qml` with `use24Hour` state and persistent disk storage in `~/.config/dwm/clock-format.conf`.
- [x] 9.5 Bind clock pill in `DwmPanel.qml`: Left-click toggles calendar popup; Right-click toggles 12h/24h time format.
- [x] 9.6 Wire `qs.calendar` into `shell.qml` with exclusive popup handling.

### Phase 10: Desktop Update System & Automated Release Pipeline
- [x] 10.1 Add `install-system` target to `Makefile` to generate system installation receipt `/usr/local/share/dwm/desktop-install.json`.
- [x] 10.2 Add `@PREFIX@` substitution for `dwm-desktop-update-root` libexec helper.
- [x] 10.3 Create `scripts/dwm-packages.sh` package sync helper.
- [x] 10.4 Fix `record_system()` in `scripts/dwm-desktop-update` to filter non-existent asset directories.
- [x] 10.5 Update `install.sh` to run `sudo make install-system` and `python3 scripts/dwm-desktop-update record-user .`.
- [x] 10.6 Modernize `.github/workflows/create-release.yml` with checkout v4, full commit changelog generation, SHA256 checksums, and `vYYYY.MM.DD` tagging convention.

### Phase 11: Professional Documentation (mdBook) & Repository Housekeeping
- [x] 11.1 Convert documentation to mdBook structure in `docs/src/` with `docs/book.toml`.
- [x] 11.2 Fix mdBook URL fragment parsing in `docs/src/SUMMARY.md` to prevent stray anchor file generation.
- [x] 11.3 Remove redundant legacy doc files (`docs/CONFIGURATION.md`, `docs/FEATURES.md`, `docs/INSTALL.md`, `docs/SECURITY.md`, `docs/THEMING.md`).
- [x] 11.4 Create comprehensive `docs/src/updates.md` chapter covering updates, receipts, and releases.
- [x] 11.5 Update `docs/src/features.md`, `docs/src/install.md`, `README.md`, and `SECURITY.md`.
- [x] 11.6 Remove dead test scripts (`scripts/stmictst`, `scripts/dbus`, `scripts/disable-powersaving`, `scripts/pkg-scan.py`).
- [x] 11.7 Clean `scripts/__pycache__/` and add `compile_commands.json` and `release/` to `.gitignore`.
- [x] 11.8 Add `.github/workflows/deploy-docs.yml` GitHub Pages deployment workflow.

