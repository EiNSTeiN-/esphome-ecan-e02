#!/usr/bin/env bash
set -euo pipefail

section() {
  printf '\n== %s ==\n' "$1"
}

section "Kernel"
uname -a

section "User"
id
groups

section "USB devices"
if command -v lsusb >/dev/null 2>&1; then
  lsusb
else
  echo "lsusb not installed"
fi

section "USB topology"
if command -v lsusb >/dev/null 2>&1; then
  lsusb -t
else
  echo "lsusb not installed"
fi

section "Serial modules"
for module in cdc_acm usbserial ch341 ch343; do
  if modinfo "$module" >/dev/null 2>&1; then
    echo "$module: available"
  else
    echo "$module: not available"
  fi
done

section "Loaded serial modules"
if command -v lsmod >/dev/null 2>&1; then
  lsmod | awk 'NR == 1 || $1 ~ /^(cdc_acm|usbserial|ch341|ch343)$/'
else
  echo "lsmod not installed"
fi

section "Modprobe blacklists"
if command -v rg >/dev/null 2>&1; then
  rg -n '(^|\s)(blacklist|install)\s+(ch341|usbserial|cdc_acm|ch343)\b|1a86|303a|55d[0-9a-f]|7523|5523|1001' \
    /etc/modprobe.d /usr/lib/modprobe.d /lib/modprobe.d || true
else
  grep -RInE '(^|[[:space:]])(blacklist|install)[[:space:]]+(ch341|usbserial|cdc_acm|ch343)\b|1a86|303a|55d[0-9a-f]|7523|5523|1001' \
    /etc/modprobe.d /usr/lib/modprobe.d /lib/modprobe.d 2>/dev/null || true
fi

section "ESPHome detected port"
if port="$(./scripts/serial-port.sh 2>/dev/null)"; then
  echo "$port"
else
  echo "No ESPHome serial port auto-detected"
fi

section "TTY candidates"
found=false
for pattern in /dev/serial/by-id/* /dev/ttyUSB* /dev/ttyACM* /dev/ttyCH*; do
  for path in $pattern; do
    if [ -e "$path" ]; then
      found=true
      ls -l "$path"
    fi
  done
done
if [ "$found" = false ]; then
  echo "No USB serial tty candidates visible in this environment"
fi

section "Recent USB serial kernel messages"
if command -v journalctl >/dev/null 2>&1; then
  journalctl -k --no-pager -n 160 |
    grep -Ei 'usb|tty(USB|ACM)|ch341|ch343|cdc_acm|303a|1a86|55d[0-9a-f]|7523|5523|1001' || true
else
  echo "journalctl not installed"
fi
