#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    helper = directory / 'probe.c'
    library = directory / 'probe.so'
    helper.write_text('''
#include "engine/node.h"
#include <assert.h>

double probe_sample(Node *node) {
  const double sample_rate = 48000;
  assert(node && node->frame_perform);
  Node *inputs[MAX_INPUTS] = {0};
  for (int i = 0; i < node->num_inputs; i++) {
    inputs[i] = (Node *)node->connections[i].source_node_index;
  }
  node->frame_perform(node, node->state_ptr, inputs, 0, 1.0 / sample_rate);
  return node->output.buf[0];
}
''')
    subprocess.run(['cc', '-shared', '-fPIC', '-I.', str(helper),
                    '-o', str(library)], check=True)
    source = f'''
open test/fixtures/audio_jit/DSP_noinit;
let _ = dlopen "{library}";
let probe_sample = extern fn Ptr -> Double;
let tone = @Audio fn freq dur -> freq + dur * 100.;;
let defaults = [|55., 4.|];
let make = synth_default defaults tone;
let original = make ([]);
print `empty={{probe_sample original == 455.}}\\n`;
print `override={{probe_sample (make [("freq", 39.), ("dur", 1.)]) == 139.}}\\n`;
print `reset={{probe_sample (make ([("freq", 46.)])) == 446.}}\\n`;
print `captured={{defaults[0] == 55. && defaults[1] == 4.}}\\n`;
print `duplicate={{probe_sample (make [("dur", 2.), ("dur", 3.)]) == 355.}}\\n`;
print `unknown={{probe_sample (make ([("missing", 9.)])) == 455.}}\\n`;
print `original={{probe_sample original == 455.}}\\n`;
print `empty_again={{probe_sample (make ([])) == 455.}}\\n`;
'''
    path = directory / 'defaults.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    for name in ('empty', 'override', 'reset', 'captured', 'duplicate',
                 'unknown', 'original', 'empty_again'):
        assert f'{name}=True' in output, output
    print('Synth defaults regression passed')
PY
