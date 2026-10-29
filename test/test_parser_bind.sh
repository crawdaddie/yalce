#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

script = 'test/test_scripts/31_parser_bind.ylc'
result = subprocess.run(['build/ylc', '--verify-ir', '--test', script],
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '3 / 3 passed' in result.stdout, result.stdout + result.stderr
calls = [
    'match pair "34!" with | Some ((\'3\', \'4\'), rest) -> rest == "!" | _ -> false',
    'match pair "3" with | None -> true | _ -> false',
    'match triple "345!" with | Some ((\'3\', \'4\', \'5\'), rest) -> rest == "!" | _ -> false',
]
session = ''.join(f'if ({call}) then print "PASS\\n" else print "FAIL\\n";\n' for call in calls)
result = subprocess.run(['build/ylc', '-i', script],
                        input=session + '%quit\n',
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and result.stdout.count('PASS\n') == len(calls), result.stdout + result.stderr
print('Parser bind passed: script and repeated REPL calls')
PY
