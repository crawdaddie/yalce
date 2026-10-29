#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

script = 'test/test_scripts/27_opt_forward_capture.ylc'
result = subprocess.run(['build/ylc', '--test', script], capture_output=True, text=True)
assert result.returncode == 0 and '2 / 2 passed' in result.stdout, result.stdout + result.stderr
session = 'if curried 5 == Some 12 then print "PASS\\n" else print "FAIL\\n";\nif captured 5 == Some 12 then print "PASS\\n" else print "FAIL\\n";\nif captured 8 == Some 15 then print "PASS\\n" else print "FAIL\\n";\n%quit\n'
result = subprocess.run(['build/ylc', '-i', script], input=session, capture_output=True, text=True)
assert result.returncode == 0 and result.stdout.count('PASS\n') == 3, result.stdout + result.stderr
print('Forwarded captures passed: script and repeated REPL calls')
PY
