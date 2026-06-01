#!/usr/bin/env bash
set -euo pipefail

if [ -n "${ESPHOME_PORT:-}" ]; then
  echo "$ESPHOME_PORT"
  exit 0
fi

if [ -d /dev/serial/by-id ]; then
  while IFS= read -r path; do
    name="$(basename "$path" | tr '[:upper:]' '[:lower:]')"
    case "$name" in
      *espressif*|*jtag*|*acm*|*qinheng*|*wch*|*ch34*|*ch343*|*ch341*|*usb-serial*|*usb_serial*|*serial*)
        readlink -f "$path"
        exit 0
        ;;
    esac
  done < <(find /dev/serial/by-id -maxdepth 1 -type l | sort)

  first="$(find /dev/serial/by-id -maxdepth 1 -type l | sort | head -n 1 || true)"
  if [ -n "$first" ]; then
    readlink -f "$first"
    exit 0
  fi
fi

for path in /dev/ttyUSB* /dev/ttyACM* /dev/ttyCH343USB* /dev/ttyCH*; do
  if [ -e "$path" ]; then
    echo "$path"
    exit 0
  fi
done

echo "No USB serial port found. Pass a port explicitly, for example: ./scripts/flash-bare.sh /dev/ttyUSB0, /dev/ttyACM0, or /dev/ttyCH343USB0" >&2
exit 1
