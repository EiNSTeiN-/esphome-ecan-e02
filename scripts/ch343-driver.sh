#!/usr/bin/env bash
set -euo pipefail

repo_url="https://github.com/WCHSoftGroup/ch343ser_linux.git"
driver_dir="${CH343_DRIVER_DIR:-/tmp/ch343ser_linux-wch}"
command="${1:-build}"

fetch_driver() {
  if [ -d "$driver_dir/.git" ]; then
    git -C "$driver_dir" fetch origin
    git -C "$driver_dir" pull --ff-only
  else
    git clone "$repo_url" "$driver_dir"
  fi
}

build_driver() {
  fetch_driver
  make -C "$driver_dir/driver"
}

run_root() {
  if command -v pkexec >/dev/null 2>&1; then
    pkexec "$@"
  else
    sudo "$@"
  fi
}

case "$command" in
  fetch)
    fetch_driver
    ;;
  build)
    build_driver
    ;;
  load)
    build_driver
    if [ -d /sys/module/ch343 ]; then
      echo "ch343 is already loaded"
    else
      run_root insmod "$driver_dir/driver/ch343.ko"
    fi
    ;;
  unload)
    run_root rmmod ch343
    ;;
  install)
    build_driver
    run_root make -C "$driver_dir/driver" install
    ;;
  status)
    if [ -d /sys/module/ch343 ]; then
      echo "ch343 is loaded"
    else
      echo "ch343 is not loaded"
    fi
    if [ -e "$driver_dir/driver/ch343.ko" ]; then
      modinfo "$driver_dir/driver/ch343.ko"
    fi
    ;;
  *)
    echo "Usage: $0 {fetch|build|load|unload|install|status}" >&2
    exit 2
    ;;
esac

