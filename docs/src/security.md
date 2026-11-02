# Security notes

This is an X11 desktop environment. X11 clients generally share broad access to the display session; DWM and Quickshell do not provide isolation between mutually untrusted applications.

## Secrets

Weather credentials, when used, belong in `~/.config/dwm/secrets/weather.env`. Do not commit credentials. Restrict access to the file (for example, `chmod 600 ~/.config/dwm/secrets/weather.env`) and avoid putting secrets in command lines or public logs.

## Runtime and commands

Project scripts use private runtime/config paths for operations that handle sensitive state. TOML `cmd` hotkeys execute through a shell; prefer `exec=[...]` for fixed commands and quote/validate data used in shell commands.

Privileged system actions use Polkit where supported and may display an authentication prompt. Keep a Polkit authentication agent running in the desktop session. Review prompts before authorizing an operation.

Report suspected vulnerabilities privately through GitHub's security reporting feature when enabled, or contact the maintainers privately. Avoid posting exploit details in public issues.
