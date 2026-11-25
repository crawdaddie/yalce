#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

# Drain the tracker synchronously to test playback without an audio device.
source = '''open test/fixtures/audio_jit/DSP_noinit;
import Tracker;
let tone = @Audio fn freq -> sin_osc freq;;
let make = synth_default [|55.|] tone;
let handle = fn ctx dur note instr params ->
  make [("freq", note |> Double |> midicps),] |> play_node;
  print `event={instr}\\n`
;;
let pattern = Tracker.parse_pattern "00;\\n00;\\n00;\\n00;\\n00;\\n00;\\n";
let co = pattern
  |> Tracker.pattern_cor_ctx (bpm: 180., spl: 0.25, logical_time: 0.) 0
  |> cor_map (Tracker.handle_ev_ctx handle)
  |> cor_repeat 2;
for i = 0 .. 13 in (
  match co () with
  | Some dur -> print `duration={dur}\\n`
  | None -> print "done\\n"
);
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'tracker.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert result.stdout.count('event=0\n') == 12, output
    assert result.stdout.count('duration=0.250000\n') == 12, output
    assert result.stdout.count('done\n') == 1, output
print('Tracker playback regression passed')

# Sparse columns must sustain independently, including the opening chord.
source = '''open test/fixtures/audio_jit/DSP_noinit;
import Tracker;
let handle = fn ctx dur note instr params ->
  print `note={note} instr={instr} dur={dur}\\n`
;;
let pattern = Tracker.parse_pattern `
00a#1 ;01f#5;



00f2  ;01f#5;

      ;01d#5;01f5;

00f#2;


00d#2;
00a#2;



`;
let co = pattern
  |> Tracker.pattern_cor_ctx (bpm: 120., spl: 1., logical_time: 0.) 0
  |> cor_map (Tracker.handle_ev_ctx handle)
  |> cor_repeat 2;
for i = 0 .. 13 in (
  match co () with
  | Some dur -> print `wait={dur}\\n`
  | None -> print "done\\n"
);
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'columns.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    expected = [(34, 0, 4), (78, 1, 4), (41, 0, 4), (78, 1, 2),
                (75, 1, 10), (77, 1, 10), (42, 0, 3), (39, 0, 1), (46, 0, 4)]
    events = [line for line in result.stdout.splitlines() if line.startswith('note=')]
    assert events == [f'note={note} instr={instr} dur={dur:.6f}'
                      for note, instr, dur in expected] * 2, output
    waits = [line for line in result.stdout.splitlines() if line.startswith('wait=')]
    assert waits == [f'wait={dur:.6f}' for dur in (4, 2, 2, 3, 1, 4)] * 2, output
    assert result.stdout.count('done\n') == 1, output
print('Tracker sparse-column regression passed')
PY
