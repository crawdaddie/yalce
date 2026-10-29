#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

session = '''type GpuStatus = | GpuDone;
Some GpuDone;
type Envelope = | Packed of GpuStatus;
Some (Packed GpuDone);
%quit
'''
result = subprocess.run(['build/ylc', '-i'], input=session,
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0, result.stderr
for expected in [
    'Option\n  | Some of GpuStatus\n    | GpuDone\n  | None',
    'Option\n  | Some of Envelope\n    | Packed of GpuStatus\n      | GpuDone\n  | None',
]:
    assert expected in result.stdout, result.stdout
print('Nested type indentation passed')
PY
