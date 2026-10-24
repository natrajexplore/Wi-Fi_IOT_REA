#!/usr/bin/env bash
#
# dashboard.sh — tiny live view of the sensor feed on your own broker.
#
# Subscribes to the telemetry topic and prints each message with a timestamp, so
# you can watch genuine readings and (during the break phase) the spoofed
# temp_c=999 messages arrive side by side.
#
# Usage:
#   ./dashboard.sh [HOST] [TOPIC] [USER] [PASS] [PORT]
#
# Defaults match secrets.example.h / provision.ino:
#   HOST=192.168.1.50  TOPIC='lab/sensor01/telemetry'  USER=sensor01
#   PASS=sensor-lab-pass  PORT=1883
#
# Requires mosquitto_sub (from the mosquitto-clients package).
#
set -euo pipefail

HOST="${1:-192.168.1.50}"
TOPIC="${2:-lab/sensor01/telemetry}"
USER="${3:-sensor01}"
PASS="${4:-sensor-lab-pass}"
PORT="${5:-1883}"

if ! command -v mosquitto_sub >/dev/null 2>&1; then
  echo "error: mosquitto_sub not found. Install the mosquitto-clients package." >&2
  echo "  Debian/Ubuntu:  sudo apt install mosquitto-clients" >&2
  echo "  macOS (brew):   brew install mosquitto" >&2
  exit 1
fi

echo ">> subscribing to $TOPIC on $HOST:$PORT  (Ctrl-C to stop)"
echo ">> watch for temp_c=999 / \"spoofed\":true during the break phase."
echo

# -v prints "topic payload"; prefix each line with a local timestamp.
mosquitto_sub -h "$HOST" -p "$PORT" -u "$USER" -P "$PASS" -t "$TOPIC" -v \
  | while IFS= read -r line; do
      printf '%s  %s\n' "$(date '+%H:%M:%S')" "$line"
    done
