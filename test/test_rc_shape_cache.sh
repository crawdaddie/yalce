#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import re
import subprocess

result = subprocess.run(
    ['build/ylc', '--profile-lowering', '--verify-ir', '--test',
     'test/test_scripts/32_closure_drop_layout.ylc'],
    capture_output=True, text=True, timeout=30)
assert result.returncode == 0 and '3 / 3 passed' in result.stdout, result.stdout + result.stderr
total = next(line for line in result.stderr.splitlines()
             if line.startswith('[lowering-rc]') and ' total ' in line)
match = re.search(r'shape-hits=(\d+)', total)
assert match and int(match[1]) > 0, total
print(total)
print('Structural cache reused copied types; closure tests passed')
PY
