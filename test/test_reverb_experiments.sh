#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from array import array
import math
from pathlib import Path
import subprocess
import tempfile

SAMPLE_RATE = 48000
SECONDS = 8
CHANNELS = 2

with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    helper = directory / 'render.c'
    library = directory / 'render.so'
    helper.write_text('''
#include "engine/ctx.h"
#include "engine/engine_core.h"
#include "lang/runtime/ylc_stdlib.h"
#include <assert.h>
#include <stdio.h>

int render_probe(Node *node, _String path) {
  const int sample_rate = 48000;
  const int seconds = 8;
  const int channels = 2;
  init_ctx();
  ctx.sample_rate = sample_rate;
  ctx.spf = 1.0 / sample_rate;
  Node *buffer = render_to_buf(sample_rate * seconds, node);
  assert(buffer && buffer->output.layout == channels);
  FILE *file = fopen(path.chars + path.offset, "wb");
  assert(file);
  size_t samples = (size_t)sample_rate * seconds * channels;
  assert(fwrite(buffer->output.buf, sizeof(double), samples, file) == samples);
  assert(fclose(file) == 0);
  return 0;
}
''')
    subprocess.run(['cc', '-shared', '-fPIC', '-I.', str(helper),
                    '-Lengine/build', '-lyalce_synth',
                    '-Wl,-rpath,' + str(Path('engine/build').resolve()),
                    '-o', str(library)], check=True)
    source = Path('reverb.ylc').read_text().split('# Run this file')[0].replace(
        'open libs/audio_jit/DSP;', 'open test/fixtures/audio_jit/DSP_noinit;')
    source += f'\nlet _ = dlopen "{library}";\n'
    source += 'let render_probe = extern fn Ptr -> String -> Int;\n'
    cases = {
        'dry': 'let signal = sin_osc 500. * decay 0.1 (trig 1.); ~[signal, signal]',
        'cloud_level': 'sin_osc 500. * decay 0.1 (trig 1.) |> Space.cloud 15. 7000. 0.',
        'ambient_level': 'sin_osc 500. * decay 0.1 (trig 1.) |> Space.ambient 15. 7000. 0.',
        'shimmer_level': 'sin_osc 500. * decay 0.1 (trig 1.) |> Space.shimmer 15. 7000. 12. 0.8 0.',
        'hall_level': 'sin_osc 500. * decay 0.1 (trig 1.) |> Space.hall 8. 5500. 0.6',
        'cloud': 'trig 0 |> Space.cloud 1.5 10000. 0.',
        'ambient': 'trig 0 |> Space.ambient 3. 8000. 0.',
        'hall': 'trig 0 |> Space.hall 3. 8000. 0.6',
        'shimmer': 'trig 0 |> Space.shimmer 4. 8000. 12. 1. 0.',
        'silence': 'Space.cloud 60. 16000. 1. 0.',
        'frozen': '''let age = sweep 1. (trig 0);
          let hold = if age >= 1. then 1. else 0.;
          trig 0 |> Space.cloud 3. 10000. hold''',
        'blocked': '''let age = sweep 1. (trig 0);
          let hold = if age >= 1. then 1. else 0.;
          let extra = if age >= 2. && age < 2.01 then 10. else 0.;
          (trig 0 + extra) |> Space.cloud 3. 10000. hold''',
        'released': '''let age = sweep 1. (trig 0);
          let hold = if age >= 1. && age < 3. then 1. else 0.;
          trig 0 |> Space.cloud 1.5 10000. hold''',
        'shimmer_stress': '''(sin_osc 110. + sin_osc 997.) * 0.1
          |> Space.shimmer 60. 16000. 12. 1. 0.''',
        'shimmer_tone': 'sin_osc 500. * 0.1 |> Space.shimmer 15. 7000. 12. 1. 0.',
        'shimmer_off': 'sin_osc 500. * 0.1 |> Space.shimmer 15. 7000. 12. 0. 0.',
    }
    for name, body in cases.items():
        source += (f'let probe_{name} = @Audio fn () -> {body};;\n'
                   f'render_probe (probe_{name} ()) "{directory / name}";\n')
    path = directory / 'reverbs.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=90)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    renders = {}
    for name in cases:
        samples = array('d')
        samples.frombytes((directory / name).read_bytes())
        assert len(samples) == SAMPLE_RATE * SECONDS * CHANNELS, name
        assert all(math.isfinite(sample) for sample in samples), name
        assert max(map(abs, samples)) < 2., name
        renders[name] = samples

    def energy(samples, start, end):
        return math.fsum(sample * sample for sample in
                         samples[int(start * SAMPLE_RATE) * CHANNELS:
                                 int(end * SAMPLE_RATE) * CHANNELS])

    # Keep the demo's wet RMS within 20 dB of its dry source.
    minimum_energy = energy(renders['dry'], 4., 8.) * 0.01
    for name in ('cloud_level', 'ambient_level', 'shimmer_level', 'hall_level'):
        assert energy(renders[name], 4., 8.) >= minimum_energy, name

    def tone_power(samples, frequency):
        signal = samples[4 * SAMPLE_RATE * CHANNELS::CHANNELS]
        phase = 2. * math.pi * frequency / SAMPLE_RATE
        real = math.fsum(x * math.cos(phase * i) for i, x in enumerate(signal))
        imag = math.fsum(x * math.sin(phase * i) for i, x in enumerate(signal))
        return real * real + imag * imag

    # An octave-up setting must produce an audible octave, beyond startup leakage.
    octave = tone_power(renders['shimmer_tone'], 1000.)
    fundamental = tone_power(renders['shimmer_tone'], 500.)
    assert octave > fundamental * 0.01, (octave, fundamental)
    assert octave > tone_power(renders['shimmer_off'], 1000.) * 100.

    for name in ('cloud', 'ambient', 'hall', 'shimmer'):
        samples = renders[name]
        assert energy(samples, 0.1, 1.) > 1e-8, name
        assert energy(samples, 6., 8.) < energy(samples, 0.1, 2.), name
        assert all(sample == 0. for sample in samples[:int(0.02 * SAMPLE_RATE) * CHANNELS]), name
        width = math.fsum((left - right) ** 2
                         for left, right in zip(samples[::2], samples[1::2]))
        assert width > 1e-8, name
    assert all(sample == 0. for sample in renders['silence'])
    assert renders['frozen'] == renders['blocked'], 'input leaked into freeze'
    early = energy(renders['frozen'], 2., 3.)
    late = energy(renders['frozen'], 6., 7.)
    assert early > 1e-8 and 0.5 < late / early < 2., (early, late)
    assert energy(renders['released'], 6., 7.) < late * 0.1
    print('Reverb level, octave shift, decay, stereo, freeze, and stress checks passed')
PY
