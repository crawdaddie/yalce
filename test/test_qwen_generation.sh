#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

root = Path.cwd()
source = (root / 'loadqwen.ylc').read_text()
source = source.replace('"libs/hf_tokenizer/target/release/libhf_tokenizer.so"',
                        '"' + str(root / 'libs/hf_tokenizer/target/release/libhf_tokenizer.so') + '"')
with tempfile.TemporaryDirectory() as directory:
    for limit, eos in [(16, 151643), (0, 151643), (4, 358)]:
        case = re.sub(r'let max_new_tokens = \d+;', f'let max_new_tokens = {limit};', source, count=1)
        case = re.sub(r'let eos_token_id = \d+;', f'let eos_token_id = {eos};', case, count=1)
        path = Path(directory) / 'generation.ylc'
        path.write_text(case)
        calls = 2 if limit == 16 else 1
        session = 'chat "blah blah blah...";\n' * calls + '%quit\n'
        result = subprocess.run([str(root / 'build/ylc'), '-i', str(path)],
                                input=session, capture_output=True, text=True, timeout=180)
        assert result.returncode == 0, result.stdout + result.stderr
        assert '\x1b_G' not in result.stdout, 'Plots must stay disabled'
        steps = re.findall(r'^token (\d+)/(\d+): (\d+)$', result.stdout, re.M)
        if limit == 16:
            assert len(steps) == limit * calls, result.stdout
            assert steps[0][2] == '358', result.stdout
            assert [int(step[0]) for step in steps] == list(range(1, limit + 1)) * calls
            assert result.stdout.count('generated text: " I\'m not sure if this is the right place to ask this question, but"') == calls, result.stdout
        else:
            assert not steps, result.stdout
            assert 'generated text: ""' in result.stdout, result.stdout
        print(f'Generation passed: limit={limit}, EOS={eos}')
PY
