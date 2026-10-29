#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

script = 'test/test_scripts/29_constructor_values.ylc'
result = subprocess.run(['build/ylc', '--test', script],
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '7 / 7 passed' in result.stdout, result.stdout + result.stderr
calls = [
    'match project 7 with | Some (ProjectWeight n) -> n == 7 | _ -> false',
    'match construct 8 with | ProjectWeight n -> n == 8 | _ -> false',
    'match boxed "again" with | Some (Some text) -> text == "again" | _ -> false',
    'match Some 9 <&> ProjectWeight with | Some (ProjectWeight n) -> n == 9 | _ -> false',
]
session = ''.join(f'if ({call}) then print "PASS\\n" else print "FAIL\\n";\n' for call in calls * 2)
result = subprocess.run(['build/ylc', '-i', script], input=session + '%quit\n',
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and result.stdout.count('PASS\n') == len(calls) * 2, result.stdout + result.stderr
print('Constructor values passed: script and repeated REPL calls')
PY
