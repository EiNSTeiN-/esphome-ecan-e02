#!/usr/bin/env bash
set -euo pipefail

seconds="${1:-60}"

echo "Watching USB/TTY kernel messages for ${seconds}s."
echo "Replug the ESP32 native USB port or the external CH343 adapter now."

if command -v journalctl >/dev/null 2>&1; then
  timeout "${seconds}s" journalctl -k -f -n 0 --no-pager |
    grep --line-buffered -Ei 'usb|tty(USB|ACM|CH)|ch341|ch343|cdc_acm|303a|1a86|55d[0-9a-f]|7523|5523|1001' || true
elif command -v udevadm >/dev/null 2>&1; then
  timeout "${seconds}s" udevadm monitor --kernel --subsystem-match=usb --subsystem-match=tty || true
else
  echo "Neither journalctl nor udevadm is installed; run lsusb before and after replugging."
fi
