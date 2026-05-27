#!/bin/bash
# config/polybar/scripts/football_ticker.sh - Sliding ticker for Polybar

CACHE_FILE="$HOME/.cache/football_matches.cache"
WIDTH=80
POS=0

while true; do
    if [ -f "$CACHE_FILE" ]; then
        MATCHES=$(cat "$CACHE_FILE")
        if [ -n "$MATCHES" ]; then
            MATCHES="$MATCHES | " # Add padding for wrap-around
            LEN=${#MATCHES}
            
            # Extract slice
            SLICE="${MATCHES:$POS:$WIDTH}"
            
            # Wrap around if slice is too short
            if [ ${#SLICE} -lt $WIDTH ]; then
                REMAINING=$((WIDTH - ${#SLICE}))
                SLICE="${SLICE}${MATCHES:0:$REMAINING}"
            fi
            
            echo "⚽ $SLICE"
            POS=$(( (POS + 1) % LEN ))
        else
            echo "" # Nothing to show
        fi
    else
        echo ""
    fi
    sleep 0.2
done
