#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

script = 'test/test_scripts/30_opt_parser_capture.ylc'
result = subprocess.run(['build/ylc', '--verify-ir', '--test', script],
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '2 / 2 passed' in result.stdout, result.stdout + result.stderr
call = 'if (match pair "34!" with | Some ((\'3\', \'4\'), rest) -> rest == "!" | _ -> false) then print "PASS\\n" else print "FAIL\\n";\n'
result = subprocess.run(['build/ylc', '-i', script],
                        input=call * 3 + '%quit\n',
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and result.stdout.count('PASS\n') == 3, result.stdout + result.stderr
print('Parser captures passed: script and repeated REPL calls')
PY
