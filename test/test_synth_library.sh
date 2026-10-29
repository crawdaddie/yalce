#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Exercise the linked library without requiring an audio device.
python3 - <<'PY'
import ctypes
import os

synth = ctypes.CDLL('./engine/build/libyalce_synth.so', mode=os.RTLD_LAZY)
synth.get_current_sample.restype = ctypes.c_uint64
synth.get_current_sample.argtypes = []
assert synth.get_current_sample() == 0
print('Synth library smoke test passed')
PY
