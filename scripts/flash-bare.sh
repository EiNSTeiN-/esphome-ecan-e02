#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
port="${1:-$("$repo_dir/scripts/serial-port.sh")}"

exec "$repo_dir/scripts/esphome.sh" upload "$repo_dir/configs/ecan-e02-bare.yaml" --device "$port"

