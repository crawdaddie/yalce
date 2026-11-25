#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

# Render the actual preset without opening an audio device.
with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    helper = directory / 'probe.c'
    library = directory / 'probe.so'
    helper.write_text('''
#include "engine/ctx.h"
#include "engine/engine_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

double voice_peak(Node *node) {
  const int sample_rate = 48000;
  const int frames = sample_rate / 2;
  init_ctx();
  ctx.sample_rate = sample_rate;
  ctx.spf = 1.0 / sample_rate;

  Node *buffer = render_to_buf(frames, node);
  assert(buffer);
  double peak = 0.0;
  for (int i = 0; i < frames * buffer->output.layout; i++) {
    double sample = buffer->output.buf[i];
    assert(isfinite(sample));
    if (i >= frames * buffer->output.layout / 2) {
      peak = fmax(peak, fabs(sample));
    }
  }
  printf("Sustained peak: %g\\n", peak);
  return peak;
}
''')
    subprocess.run(['cc', '-shared', '-fPIC', '-I.', str(helper),
                    '-Lengine/build', '-lyalce_synth', '-lm',
                    '-Wl,-rpath,' + str(Path('engine/build').resolve()),
                    '-o', str(library)], check=True)
    source = Path('a4.ylc').read_text().split('\nA4.bass ')[0]
    source = source.replace('open libs/audio_jit/DSP;',
                            'open test/fixtures/audio_jit/DSP_noinit;')
    source += f'''
let _ = dlopen "{library}";
let voice_peak = extern fn Ptr -> Double;
let peak = A4.bass 55. 4. 0. 0.6 700. 0.4 -0.2 |> voice_peak;
print `bass_audible={{peak > 0.01}}\\n`;
let lead_peak = A4.sync_lead 110. 4. 19. 1. 4000. |> voice_peak;
print `lead_audible={{lead_peak > 0.01}}\\n`;
'''
    path = directory / 'a4_probe.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--base', str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert 'bass_audible=True' in output, output
    assert 'lead_audible=True' in output, output
    print(output.strip())
PY
