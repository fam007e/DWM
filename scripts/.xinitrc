#!/bin/bash

# .xinitrc — startx entry point for dwm
# All setup (DPMS, dbus env, services) is handled by autostart.sh

# Guard: only create a new dbus session if one isn't already active
# (prevents nested/double bus if startx is run from an existing session)
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    exec dbus-run-session dwm
else
    exec dwm
fi
