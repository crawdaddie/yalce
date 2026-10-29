#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

source = Path('loadqwen.ylc').read_text()
qkv = source[source.index('let Qkv = module'):source.index('let Rope = module')]
keep = source[source.index('  let keep ='):source.index('  # Temporary resources')]
release = source[source.index('  let release ='):source.index('  let upload =')].replace(': (Uint64)', ': (Int)')
fixture = '''open std/Opt;
type Calculation = (data: Int, rows: Int, width: Int);
type GpuStatus = | GpuDone;
let fail = [| 0 |];
let freed = [| 0, 0, 0 |];
let count = [| 0 |];
let as_qkv = fn value -> value;;
let as_calculation = fn value: (Calculation) -> value;;
let Gpu = module () ->
  let free = fn data ->
    freed[count[0]] := data;
    count[0] := count[0] + 1;
    Some GpuDone
  ;;
''' + keep + release + ''';
open Gpu;
let Linear = module () ->
  let apply = fn layer value: (Calculation) ->
    if layer == fail[0] then None else Some (as_calculation (layer, value.rows, value.width))
  ;;
;
''' + qkv
with tempfile.TemporaryDirectory() as directory:
    for failure, expected in [(0, [0]), (1, [0]), (2, [1, 0]), (3, [2, 1, 0])]:
        checks = ' && '.join(f'freed[{i}] == {value}' for i, value in enumerate(expected))
        outcome = 'q.data == 1 && k.data == 2 && v.data == 3' if failure == 0 else 'false'
        none = 'false' if failure == 0 else 'true'
        case = fixture + f'''
fail[0] := {failure};
let result = Qkv.forward (1, 2, 3) (0, 1, 1);
let valid = match result with
| Some (q, k, v) -> {outcome}
| None -> {none};
if valid && count[0] == {len(expected)} && {checks}
then print "PASS\\n" else print "FAIL\\n";
'''
        path = Path(directory) / 'qkv.ylc'
        path.write_text(case)
        result = subprocess.run(['build/ylc', str(path)], capture_output=True, text=True, timeout=30)
        assert result.returncode == 0 and 'PASS\n' in result.stdout, result.stdout + result.stderr
print('QKV ownership passed: success and each projection failure')
PY
