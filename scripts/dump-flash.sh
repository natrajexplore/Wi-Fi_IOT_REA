#!/usr/bin/env bash
#
# dump-flash.sh — read the full flash image off your own ESP32 for analysis.
#
# Usage:
#   ./dump-flash.sh [PORT] [SIZE] [OUTFILE]
#
#   PORT     serial port of the board   (default: auto-detect common names)
#   SIZE     flash size in hex bytes     (default: 0x400000 = 4 MB, classic ESP32)
#   OUTFILE  output file                 (default: dump.bin)
#
# Examples:
#   ./dump-flash.sh                          # 4 MB, auto port, -> dump.bin
#   ./dump-flash.sh /dev/ttyUSB0 0x800000 dump-v2.bin   # 8 MB S3, hardened image
#
# Only run this against a board you own. The dump is gitignored (*.bin).
#
set -euo pipefail

PORT="${1:-}"
SIZE="${2:-0x400000}"
OUT="${3:-dump.bin}"

# esptool ships as `esptool` (v5+), `esptool.py` (older), or only as a Python
# module (common on Windows, where the pip scripts dir is often off PATH).
# Prefer whichever exists; store as an array so a multi-word command works.
if command -v esptool >/dev/null 2>&1; then
  ESPTOOL=(esptool)
elif command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
elif command -v python >/dev/null 2>&1 && python -m esptool version >/dev/null 2>&1; then
  ESPTOOL=(python -m esptool)
else
  echo "error: esptool not found. Install it with:  pip install esptool" >&2
  exit 1
fi

# Auto-detect a likely port if none was given.
if [ -z "$PORT" ]; then
  for cand in /dev/ttyUSB0 /dev/ttyACM0 /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART /dev/cu.usbmodem*; do
    if compgen -G "$cand" >/dev/null 2>&1; then PORT="$(ls $cand | head -n1)"; break; fi
  done
fi

if [ -z "$PORT" ]; then
  echo "error: no serial port found. Pass it explicitly:" >&2
  echo "   Linux:   ./dump-flash.sh /dev/ttyUSB0" >&2
  echo "   macOS:   ./dump-flash.sh /dev/cu.usbserial-0001" >&2
  echo "   Windows: ./dump-flash.sh COM5        (check Device Manager / 'python -m esptool flash-id')" >&2
  exit 1
fi

echo ">> chip / flash info (confirm the real size before dumping):"
"${ESPTOOL[@]}" --port "$PORT" flash-id || true
echo

echo ">> reading flash: port=$PORT size=$SIZE out=$OUT"
"${ESPTOOL[@]}" --port "$PORT" --baud 921600 read-flash 0x0 "$SIZE" "$OUT"

echo
echo ">> done. $(ls -lh "$OUT" | awk '{print $5}')  ->  $OUT"
echo ">> next: paste rea/01-analyze-prompt.md (or 03 for the hardened dump) to your agent."
