#!/usr/bin/env bash
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WAV=/tmp/yalce-sample-timing.wav

rm -f "$WAV"

set +e
set -o pipefail
tail -f /dev/null | timeout 3s "$ROOT/build/ylc" -i --base "$ROOT" \
  "$ROOT/sample_timing_probe.ylc"
status=$?
set +o pipefail
set -e

if [ "$status" -ne 124 ]; then
  exit "$status"
fi

test -s "$WAV"

YLC_TIMING_PERIOD=513 \
  python3 "$ROOT/test/check_sample_timing.py" "$WAV"
