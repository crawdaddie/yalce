#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

# Exercise the actual converter and Tracker callback without audio startup.
source = Path('a4.ylc').read_text().split('\npat |> Tracker.parse_pattern')[0]
source = source.replace('open libs/audio_jit/DSP;',
                        'open test/fixtures/audio_jit/DSP_noinit;\n'
                        'let play_node = fn node: (Ptr) -> node;;\n'
                        'let (~>) = fn node: (Ptr) bus: (Ptr) -> node;;')
source += '''
let converted = plist_conv [|('r', 2), ('x', 9), ('r', 3)|];
let valid = match converted with
  | (name, value)::(next_name, next_value)::[] ->
    name == "repeat" && value == 2. && next_name == "repeat" && next_value == 3.
  | _ -> false;
print `converted={valid}\\n`;
let empty = match plist_conv [| |] with | [] -> true | _ -> false;
print `empty={empty}\\n`;
let co = Tracker.parse_pattern "01f#5 r02;\\n01d#5;\\n"
  |> Tracker.pattern_cor_ctx (bpm: 120., spl: 1., logical_time: 0.) 0
  |> cor_map (Tracker.handle_ev_ctx handle_pat_ev);
for i = 0 .. 3 in (
  match co () with
  | Some dur -> print `wait={dur}\\n`
  | None -> print "done\\n"
);
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'a4_params.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert 'converted=True\n' in output and 'empty=True\n' in output, output
    assert result.stdout.count('wait=1.000000\n') == 2, output
    assert result.stdout.count('done\n') == 1, output
print('A4 parameter conversion and callback regression passed')
PY
