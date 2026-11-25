#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Usage: bash test/test_tracker_params.sh OUTPUT_DIR [Tracker_list.ylc]
python3 - "${1:-test/mir_artifacts/tracker_params_array}" "${2:-Tracker.ylc}" <<'PY'
from pathlib import Path
import re
import subprocess
import sys
import time

root = Path.cwd()
destination = Path(sys.argv[1]).resolve()
destination.mkdir(parents=True, exist_ok=True)
source = Path('test/fixtures/tracker_params.ylc')
(destination / source.name).write_text(source.read_text())
tracker_path = Path(sys.argv[2])
tracker = tracker_path.read_text()
(destination / tracker_path.name).write_text(tracker)
# Substitute only scheduling; retain Tracker's delayed-note capture and copy path.
assert tracker.count('play_pattern 0. (') == 1
tracker = tracker.replace('play_pattern 0. (', 'probe_queue 0. (')
probe = destination / 'probe.ylc'
fixture = source.read_text()
if tracker_path.name == 'Tracker_list.ylc':
    fixture = fixture.replace('std/Arrays', 'std/Lists').replace('Arrays.fold', 'Lists.fold')
probe.write_text(fixture.replace(
    'import Tracker;', 'let Tracker = module () ->\n' + tracker + '\n;'))
ansi = re.compile(r'\x1b\[[0-9;]*m')
checks = ('immediate', 'retained', 'timing', 'copies', 'precedence')
summary = []

for flag, filename in (('--dump-mir', 'tracker.mir'),
                       ('--dump-ir-pre', 'tracker.pre.ll'),
                       ('--dump-ir', 'tracker.ll')):
    start = time.perf_counter()
    result = subprocess.run(['./build/ylc', '-O2', '--verify-ir', flag,
                             '--base', str(root), str(probe)],
                            text=True, capture_output=True, timeout=60)
    seconds = time.perf_counter() - start
    stdout = ansi.sub('', result.stdout)
    stderr = ansi.sub('', result.stderr)
    (destination / (filename + '.log')).write_text(stdout + stderr)
    assert result.returncode == 0, f'{filename}:\n{stdout}{stderr}'
    for check in checks:
        assert f'params_{check}=True\n' in stdout, f'{filename}:\n{stdout}{stderr}'
    if flag == '--dump-mir':
        dump = stdout[stdout.index('# ylc-mir'):].split('\nparams_')[0]
    else:
        dump = stderr[stderr.index('; ModuleID ='):]
    (destination / filename).write_text(dump.rstrip() + '\n')
    summary.append(f'{filename}: {seconds:.3f}s including JIT compilation and dumping')

(destination / 'summary.txt').write_text('\n'.join(summary) + '\n')
print(f'Tracker Params checks passed. Artifacts: {destination}')
print('Inspect copy_params, handle_ev_ctx_note, delay_note, __ylc_dup, and __ylc_drop.')
PY
