#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

YLC_REPO_ROOT="$ROOT_DIR" nvim --headless -u NONE -n \
  -l "$ROOT_DIR/test/ylc_nvim_marker_notebook.lua" >/dev/null
