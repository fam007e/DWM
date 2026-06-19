#!/usr/bin/env bash

THEME="minimal"

# Determine paths dynamically for portability
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export DWM_POLYBAR_SCRIPTS="$SCRIPT_DIR/scripts"
export DWM_REPO="$(realpath "$SCRIPT_DIR/../..")"
export DWM_SCRIPTS="$DWM_REPO/scripts"

# If running from ~/.config/polybar, DWM_SCRIPTS should point to ~/.local/share/dwm/scripts
if [[ "$SCRIPT_DIR" == *".config/polybar"* ]]; then
    export DWM_SCRIPTS="$HOME/.local/share/dwm/scripts"
fi

# Kill existing polybar instances before launching new ones
killall -q polybar 2>/dev/null || true
while pgrep -u "$UID" -x polybar >/dev/null; do sleep 0.1; done

if [ -f "$HOME/.config/polybar/themes/$THEME/config.ini" ]; then
    CONFIG_DIR="$HOME/.config/polybar"
elif [ -f "$SCRIPT_DIR/themes/$THEME/config.ini" ]; then
    CONFIG_DIR="$SCRIPT_DIR"
else
    CONFIG_DIR="$HOME/.config/polybar"
fi

CONFIG_FILE="$CONFIG_DIR/themes/$THEME/config.ini"
LAPTOP_CONFIG_FILE="$CONFIG_DIR/themes/$THEME/laptop-config.ini"

if command ls /sys/class/power_supply/ 2>/dev/null | command grep -q '^BAT'; then
	CONFIG_FILE=$LAPTOP_CONFIG_FILE
	# Detect battery and adapter names for polybar battery module
	export DWM_BATTERY=$(command ls /sys/class/power_supply/ 2>/dev/null | command grep -E '^BAT[0-9]' | head -1)
	export DWM_ADAPTER=$(command ls /sys/class/power_supply/ 2>/dev/null | command grep -Ev '^BAT' | head -1)
fi

# Check if xrandr is available and get monitor list
if command -v xrandr > /dev/null 2>&1; then
    # Get list of connected monitors
    mapfile -t MONITORS < <(xrandr --query | command grep " connected" | cut -d" " -f1)
    MONITOR_COUNT=${#MONITORS[@]}
    
    # Detect primary monitor
    PRIMARY_MONITOR=$(xrandr --query | command grep " connected primary" | cut -d" " -f1)
    
    # If no primary monitor is explicitly set, use the first one
    if [ -z "$PRIMARY_MONITOR" ]; then
        PRIMARY_MONITOR=${MONITORS[0]}
        echo "No primary monitor detected, using first monitor: $PRIMARY_MONITOR"
    else
        echo "Primary monitor detected: $PRIMARY_MONITOR"
    fi
    
    echo "Detected $MONITOR_COUNT monitors: ${MONITORS[*]}"
    
    if [ $MONITOR_COUNT -eq 1 ]; then
        # Single monitor setup - launch main bar with tray and EWMH
        echo "Single monitor setup - launching main polybar with tray and EWMH on ${MONITORS[0]}"
        MONITOR=${MONITORS[0]} polybar main -c "$CONFIG_FILE" &
    else
        # Multi-monitor setup
        echo "Multi-monitor setup - EWMH and systray only on primary monitor"
        
        # Launch polybar on all connected monitors
        for monitor in "${MONITORS[@]}"; do
            if [ "$monitor" = "$PRIMARY_MONITOR" ]; then
                # Primary monitor gets the tray and EWMH
                MONITOR=$monitor polybar main -c "$CONFIG_FILE" &
                echo "Launched primary polybar with tray and EWMH on $monitor"
            else
                # Secondary monitors don't get the tray or EWMH
                MONITOR=$monitor polybar secondary -c "$CONFIG_FILE" &
                echo "Launched secondary polybar without tray on $monitor"
            fi
        done
    fi
else
    # Fallback: launch main bar if xrandr is not available
    echo "xrandr not available - launching fallback main polybar with tray"
    polybar main -c "$CONFIG_FILE" &
fi

# Wait for Polybar to be ready before returning.
# This ensures tray apps started after this script can find the tray owner.
for i in $(seq 1 30); do
    if xdotool search --class Polybar >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done
