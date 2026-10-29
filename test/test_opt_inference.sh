#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import subprocess

script = 'test/test_scripts/28_opt_inference.ylc'
result = subprocess.run(['build/ylc', '--test', script],
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '4 / 4 passed' in result.stdout, result.stdout + result.stderr
source = r'{\"data_offsets\":[0,8],\"dtype\":\"BF16\",\"shape\":[4]}'
call = f'if (match inspect "{source}" with | Some (count, stride) -> count == 4 && stride == 2 | None -> false) then print "PASS\\n" else print "FAIL\\n";\n'
result = subprocess.run(['build/ylc', '-i', script],
                        input=call * 3 + '%quit\n',
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and result.stdout.count('PASS\n') == 3, result.stdout + result.stderr
print('Callback inference passed: script and repeated REPL calls')
PY
