# Configuration

User-editable DWM configuration is stored in `~/.config/dwm/`. Quickshell configuration is installed at `~/.config/quickshell/`. DWM watches its TOML files and reloads them after changes; no C rebuild is needed for TOML edits.

## Hotkeys

Edit `~/.config/dwm/hotkeys.toml`. Entries use inline tables inside arrays (not TOML `[[rules]]` tables):

```toml
[vars]
terminal = "alacritty"

keys = [
  { mod="SUPER", key="Return", desc="Terminal", func="spawn", exec=["$terminal"] },
  { mod="SUPER SHIFT", key="u", desc="Update Center", func="spawn", cmd="quickshell ipc call updatecenter toggle" },
]
```

Use `exec=[...]` for argument-separated direct execution; use `cmd="..."` only when shell syntax is needed. The checked-in `config/hotkeys.toml` is the authoritative default shortcut list. Quickshell IPC commands can be found with `quickshell ipc show`.

## Window rules

Rules are inline tables in the `rules` array in `~/.config/dwm/window-rules.toml`:

```toml
rules = [
  { title="dwm settings", isfloating=1, fillworkarea=1, noswallow=1 },
  { class="Zenity", isfloating=1, noswallow=1 },
]
```

Rules match `title`, `class`, and/or `instance`; matching rules are applied in order. Common fields:

- `isfloating=1`: make a matching client floating.
- `fillworkarea=1`: keep a floating client sized to that monitor's available work area, respecting reserved bars. This is monitor-agnostic.
- `noswallow=1`: prevent terminal swallowing.
- `isterminal=1`: allow the terminal to swallow eligible child clients.
- `tags=N`: assign the client to tag number `N`.
- `monitor=N`: select a monitor number.

Settings and System Health need `fillworkarea=1` in the user's rules file. Setup preserves existing user files, so merge new defaults manually when upgrading. To inspect a client's X11 properties, use `xprop`.

## C configuration

Changes to compile-time settings in `config.def.h` require rebuilding and installing DWM:

```sh
make
sudo make install-system
```

See [installation](install.md) for user configuration deployment and session startup.
