#!/bin/bash
# config/polybar/scripts/mic_status.sh

# Reusing logic from your stmictst script
mic_info=$(pactl get-source-mute @DEFAULT_SOURCE@ 2>/dev/null)

if echo "$mic_info" | grep -q "yes"; then
    echo "󰍭" # Muted icon from your old setup
else
    echo "" # Unmuted icon from your old setup
fi
