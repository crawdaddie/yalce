#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

source = '''let Helpers = module () ->
  let increment = 1;
  let add = fn x: (Int) -> x + increment;;
  let twice = fn x: (Int) -> add (add x);;
;
let answer = fn x: (Int) -> Helpers.twice x;;
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'helpers.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '-i', str(path)],
                            input='print `{answer 40}\\n`;\nprint `{answer 41}\\n`;\n%quit\n',
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert '42' in result.stdout and '43' in result.stdout, output
    assert 'failed' not in output and 'Error' not in output, output
print('REPL module sibling calls passed')
PY
