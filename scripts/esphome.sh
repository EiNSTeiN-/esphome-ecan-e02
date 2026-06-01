#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export UV_CACHE_DIR="${UV_CACHE_DIR:-/tmp/uv-cache}"
export UV_TOOL_DIR="${UV_TOOL_DIR:-/tmp/uv-tools}"
export PLATFORMIO_CORE_DIR="${PLATFORMIO_CORE_DIR:-/tmp/platformio-core}"
export PLATFORMIO_SETTING_ENABLE_TELEMETRY="${PLATFORMIO_SETTING_ENABLE_TELEMETRY:-false}"

cd "$repo_dir"
exec uvx --from esphome esphome "$@"
