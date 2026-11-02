# Build and update

## Install from a checkout

After updating source, run the project checks and install:

```sh
make test
sudo make install-system
python3 scripts/dwm-desktop-update record-user .
```

The system install builds and installs DWM and system helpers. The user command synchronizes/records user-level files while preserving local customizations. Review output and merge changed defaults—especially `window-rules.toml`—when the user file predates new options such as `fillworkarea`.

## Start the installed binary

Installing does not replace a DWM process already running. Exit the current X session and start it again. With `startx`, ensure `~/.xinitrc` executes the intended installed path (for the default prefix, `/usr/local/bin/dwm`); a different binary earlier in `PATH` can otherwise be launched.

The Quickshell Update Center can show update information and invoke supported update actions. Availability depends on a valid project installation and the configured package providers. See its UI for operation status and recovery actions; do not assume an interrupted operation succeeded.

## Release builds

Release tags in this repository use `vYYYY.MM.DD` (for example, `v2026.11.02`). Pushing a `v*` tag triggers the binary release workflow; updates to `docs/` on `main` trigger documentation publishing. Check `.github/workflows/` and published assets for current artifacts.
