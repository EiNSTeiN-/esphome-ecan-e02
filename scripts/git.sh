#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
git_dir="$repo_dir/.git-local"

exec git --git-dir="$git_dir" --work-tree="$repo_dir" "$@"

