#!/bin/bash
# Telnet monitor with auto-reconnect.
# Usage: ./telnet_monitor.sh <host> [port]
#   host  IP address of the bridge (e.g., 192.168.1.100)
#   port  Telnet port (default: 2323)

HOST="${1:?Usage: $0 <host> [port]}"
PORT="${2:-2323}"

while true; do
    echo "[$(date '+%H:%M:%S')] Connecting to $HOST:$PORT..."
    nc -w 60 "$HOST" "$PORT"
    echo "[$(date '+%H:%M:%S')] Disconnected. Reconnecting in 2s..."
    sleep 2
done
