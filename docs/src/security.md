# Security Model & Hardening

This build prioritizes desktop security and defensive engineering, addressing attack vectors common in custom window manager setups.

---

## 🔒 Secret Isolation

API keys (such as OpenWeatherMap tokens) and geographical coordinates are **strictly isolated** from repository code:

- Secrets live in `~/.config/dwm/secrets/weather.env`.
- The `install.sh` deployment script verifies that all secret files and parent directories enforce POSIX mode `0600` (readable and writable only by the file owner).
- The `.gitignore` prevents secret files from ever being accidentally committed.

---

## 🛡️ Input Sanitization & IPC Hardening

In X11 environments, window properties (such as `WM_NAME` and `WM_CLASS`) are set by client applications. Malicious or untrusted web pages can set document titles containing shell metacharacters, newline injections, or unprintable UTF-8 sequences.

### Mitigations
1. **Title Clamping:** The IPC state bridge `dwm-quickshell-state` truncates window titles to a maximum of 256 characters to avoid buffer exhaustion.
2. **Control Character Stripping:** Line breaks (`\r`, `\n`) and key-value delimiters (`=`) are stripped before properties are piped to the Quickshell event parser.
3. **Execution Safety:** Hotkeys in `hotkeys.toml` utilize direct `execvp` array execution (`["$terminal"]`) instead of passing raw string concatenations to `/bin/sh`.

---

## 📁 Runtime Directory Sandboxing

- Temporary files created during command checks are sandboxed in `$XDG_RUNTIME_DIR/dwm` (mode `0700`).
- If `$XDG_RUNTIME_DIR` is not provided by `systemd-logind` or `elogind`, fallback paths utilize `mktemp -d` under a strict `umask 077` mask.
