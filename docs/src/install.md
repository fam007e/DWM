# Install and start

The supported automated installer targets Arch Linux. It installs dependencies, builds DWM, deploys Quickshell and helper scripts, and seeds user configuration without overwriting existing TOML files.

```sh
git clone https://github.com/fam007e/DWM.git
cd DWM
./install.sh
```

For a manual build from a checkout:

```sh
make test
sudo make install-system
python3 scripts/dwm-desktop-update record-user .
```

`make install-system` installs the binary and system helpers under the configured `PREFIX` (default `/usr/local`). `record-user` deploys/records user-level configuration. Review installer output and merge new defaults into existing files as needed; existing user TOML is intentionally preserved.

## Start X11

A display manager is optional. With `startx`, configure `~/.xinitrc` to execute the installed DWM binary. For the default prefix:

```sh
exec /usr/local/bin/dwm
```

If you need to create a D-Bus session when one is not already active, use `dbus-run-session /usr/local/bin/dwm` in that branch. Use the actual installed path if `PREFIX` differs. An unqualified `dwm` command can select another binary earlier in `PATH`.

Log in on a TTY, run `startx`, and exit DWM to return to the TTY. Then run `startx` again to start a new session with the installed binary.

## Dependencies and checks

`install.sh` checks/installs the project dependencies for Arch Linux. Quickshell, an X server, audio/network services, and utilities used by configured hotkeys are needed for their corresponding features. On another distribution, install equivalent packages and adapt the build dependencies.

Run `make test` from the repository to check C compilation, shell/Python syntax, TOML files, and portability definitions.
