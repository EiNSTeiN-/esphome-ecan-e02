#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
port="${1:-$("$repo_dir/scripts/serial-port.sh")}"
seconds="${2:-30}"
baud="${BAUD:-115200}"

export UV_CACHE_DIR="${UV_CACHE_DIR:-/tmp/uv-cache}"
export UV_TOOL_DIR="${UV_TOOL_DIR:-/tmp/uv-tools}"
export PLATFORMIO_CORE_DIR="${PLATFORMIO_CORE_DIR:-/tmp/platformio-core}"

exec uvx --from esphome python - "$port" "$seconds" "$baud" <<'PY'
import serial
import sys
import time

port = sys.argv[1]
seconds = float(sys.argv[2])
baud = int(sys.argv[3])

with serial.Serial(port, baud, timeout=0.1, dsrdtr=False, rtscts=False) as ser:
    # Direct DTR/RTS wiring can hold ESP32 strapping/reset pins active if a
    # terminal asserts modem-control lines. Keep them inactive for normal boot.
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    print(f"[serial] {port} @ {baud}: DTR/RTS idle, pulsing RTS reset", flush=True)
    ser.dtr = False
    ser.rts = True
    time.sleep(0.2)
    ser.rts = False

    print(f"[serial] reset released, reading {seconds:g}s", flush=True)
    end = time.time() + seconds
    while time.time() < end:
        data = ser.read(4096)
        if data:
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()

print("[serial] done", flush=True)
PY
