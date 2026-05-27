#!/bin/sh

# Method 1: modetest (libdrm) — TTY, X11, Wayland, no display server needed
rate=$(modetest 2>/dev/null | awk '/^  #[0-9]/{print $3}' | sort -rn | head -1 | cut -d. -f1)
[ -n "$rate" ] && echo "$rate" && exit 0

# Method 2: xrandr — X11 only
if [ -n "$DISPLAY" ]; then
    rate=$(xrandr --current 2>/dev/null | grep -oP '\d+\.\d+(?=\*)' | head -1 | cut -d. -f1)
    [ -n "$rate" ] && echo "$rate" && exit 0
fi

# Method 3: wlr-randr — wlroots Wayland
if [ -n "$WAYLAND_DISPLAY" ]; then
    rate=$(wlr-randr 2>/dev/null | grep -oP '\d+(?=\.\d+ Hz \(current\))' | head -1)
    [ -n "$rate" ] && echo "$rate" && exit 0
fi

# Method 4: fbset — last resort framebuffer
rate=$(fbset 2>/dev/null | grep -oP 'timings.*' | grep -oP '\d+' | tail -1)
[ -n "$rate" ] && echo "$rate" && exit 0

echo 60
