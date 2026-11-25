#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
import ctypes
import math
import os
from pathlib import Path
import subprocess
import tempfile

SAMPLE_RATE = 48000
DELAY_SAMPLES = 13
BUFFER_SAMPLES = 32
RENDER_SAMPLES = 4096
TOLERANCE = 1e-12

# Exercise the actual kernel without opening an audio device.
with tempfile.TemporaryDirectory() as directory:
    library = Path(directory) / 'kernels.so'
    subprocess.run(['clang', '-O2', '-shared', '-fPIC', '-Iengine', '-Ilang',
                    'libs/audio_jit/osc_kernels.c', '-lm', '-o', str(library)],
                   check=True)
    kernels = ctypes.CDLL(str(library), mode=os.RTLD_LAZY)
    allpass = kernels.ylc_audio_dl_allpass_kernel
    allpass.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_int32,
                       ctypes.c_double, ctypes.c_double, ctypes.c_double]
    allpass.restype = ctypes.c_double

    for feedback in (-0.75, 0., 0.5, 0.9):
        state = ctypes.create_string_buffer(
            2 * ctypes.sizeof(ctypes.c_int32)
            + BUFFER_SAMPLES * ctypes.sizeof(ctypes.c_double))
        impulse = [allpass(state, 1 / SAMPLE_RATE, BUFFER_SAMPLES,
                           DELAY_SAMPLES / SAMPLE_RATE, feedback,
                           float(frame == 0))
                   for frame in range(RENDER_SAMPLES)]
        # H(z) = (z^-D - g) / (1 - g*z^-D), so every frequency has unit gain.
        expected = [-feedback] + [0.] * (RENDER_SAMPLES - 1)
        for frame in range(DELAY_SAMPLES, RENDER_SAMPLES, DELAY_SAMPLES):
            expected[frame] = ((1 - feedback * feedback)
                               * feedback ** (frame // DELAY_SAMPLES - 1))
        error = max(abs(actual - target)
                    for actual, target in zip(impulse, expected))
        assert error < TOLERANCE, (feedback, error)
        energy = math.fsum(sample * sample for sample in impulse)
        assert abs(energy - 1) < TOLERANCE, (feedback, energy)

print('Delay allpass impulse response and unity gain passed')
PY
