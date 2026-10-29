#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import re
import subprocess

script = 'test/test_scripts/32_closure_drop_layout.ylc'
result = subprocess.run(['build/ylc', '--dump-ir-pre', '--verify-ir', '--test', script],
                        capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '3 / 3 passed' in result.stdout, result.stdout + result.stderr
ir = result.stdout + result.stderr

def drop_for(test):
    body = re.search(r'define i1 @test\.' + test + r'\(\).*?\n}', ir, re.S).group()
    return re.search(r'call void @(__ylc_drop_\w+)\(', body).group(1)

# Different capture layouts must never share cleanup code.
assert drop_for('test_text') != drop_for('test_number'), 'String and Int captures share a destructor'
print('Closure cleanup respects capture layouts')
PY
