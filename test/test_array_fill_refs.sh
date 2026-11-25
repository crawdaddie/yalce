#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    helper = directory / 'refs.c'
    library = directory / 'refs.so'
    helper.write_text('''
#include "lang/runtime/ylc_stdlib.h"
int ref_count(void *payload) {
  return ((YlcRcHeader *)payload - 1)->rc;
}
''')
    subprocess.run(['cc', '-shared', '-fPIC', '-I.', str(helper),
                    '-o', str(library)], check=True)
    source = f'''
let _ = dlopen "{library}";
let ref_count = extern fn Ptr -> Int;
let string_add = extern fn String -> String -> String;
let seed = string_add "seed" "";
let before = ref_count (cstr seed);
let values = array_fill_const 6 (1, seed);
let after = ref_count (cstr seed);
print `retained={{after == before + 6}}\\n`;
for i = 0 .. array_size values in values[i] := (0, "");
print `alive={{seed == "seed"}}\\n`;
let zero_before = ref_count (cstr seed);
let empty = array_fill_const 0 (1, seed);
let zero_after = ref_count (cstr seed);
print `empty_retained={{zero_after == zero_before && array_size empty == 0}}\\n`;
type Entry = Value of String | Empty;
let sum_before = ref_count (cstr seed);
let entries = array_fill_const 2 (Value seed);
let sum_after = ref_count (cstr seed);
print `sum_retained={{sum_after == sum_before + 2}}\\n`;
for i = 0 .. array_size entries in entries[i] := Empty;
let choose = fn pair flag ->
  let head, rest = pair;
  let line = if flag then head[0 .. 3] else head;
  [line]
;;
let phi_before = ref_count (cstr seed);
let lines = choose (seed, "") true;
let phi_after = ref_count (cstr seed);
print `phi_retained={{phi_after == phi_before + 1}}\\n`;
match lines with
| line::_ -> print `slice_alive={{line == "see"}}\\n`
| _ -> print "slice_alive=False\\n";
print `seed_alive={{seed == "seed"}}\\n`;
'''
    path = directory / 'fill.ylc'
    path.write_text(source)
    result = subprocess.run(['./build/ylc', '--verify-ir', '--base',
                             str(Path.cwd()), str(path)],
                            text=True, capture_output=True, timeout=30)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert 'retained=True' in output and 'alive=True' in output, output
    assert 'empty_retained=True' in output and 'sum_retained=True' in output, output
    assert 'phi_retained=True' in output and 'slice_alive=True' in output, output
print('Array fill ownership regression passed')
PY
