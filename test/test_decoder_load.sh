#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

source = Path('loadqwen.ylc').read_text()
model_types = source[source.index('type MappedTensor ='):source.index('let as_qwen =')]
projection_types = source[source.index('type Projection ='):source.index('type QkvValues =')]
start = source.index('type DecoderWeights =')
end = source.index('  let free = fn layer: (DecoderWeights)', start)
loader = source[start:end] + ';\n'
stubs = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum Kind { NORM, BIASED, UNBIASED };
enum { HIDDEN = 8, KV_WIDTH = 128, INTERMEDIATE = 4864, PARTS = 9 };
static int failure, calls, freed, valid;
static const char *names[] = {
    "input_layernorm.weight", "self_attn.q_proj", "self_attn.k_proj",
    "self_attn.v_proj", "self_attn.o_proj", "post_attention_layernorm.weight",
    "mlp.gate_proj", "mlp.up_proj", "mlp.down_proj"
};
static const int inputs[] = {HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, HIDDEN, INTERMEDIATE};
static const int outputs[] = {0, HIDDEN, KV_WIDTH, KV_WIDTH, HIDDEN, 0, INTERMEDIATE, INTERMEDIATE, HIDDEN};
static const int kinds[] = {NORM, BIASED, BIASED, BIASED, UNBIASED, NORM, UNBIASED, UNBIASED, UNBIASED};
int configure(int stage) { failure = stage; calls = freed = 0; valid = 1; return 0; }
uint64_t load_weight(const char *name, int input, int output, int kind) {
    int index = calls++;
    char expected[128];
    if (index >= PARTS) { valid = 0; return 0; }
    snprintf(expected, sizeof(expected), "model.layers.2.%s", names[index]);
    valid &= strcmp(name, expected) == 0 && input == inputs[index]
             && output == outputs[index] && kind == kinds[index];
    return calls == failure ? 0 : (uint64_t)calls;
}
int free_weight(uint64_t weight) {
    int acquired = failure ? failure - 1 : calls;
    valid &= weight == (uint64_t)(acquired - freed);
    ++freed;
    return 0;
}
int call_count(void) { return calls; }
int free_count(void) { return freed; }
int correct(void) { return valid; }
'''
prefix = '''open std/Opt;
let int64_from_int = extern fn Int -> Uint64;
let int64_str = extern fn Uint64 -> String;
let string_add = extern fn String -> String -> String;
let configure = extern fn Int -> Int;
let load_weight = extern fn Ptr -> Int -> Int -> Int -> Uint64;
let free_weight = extern fn Uint64 -> Int;
let call_count = extern fn () -> Int;
let free_count = extern fn () -> Int;
let correct = extern fn () -> Int;
let _ = configure FAILURE;
type GpuStatus = | GpuDone;
let Gpu = module () ->
  let weight = fn model: (QwenModel) name: (String) count ->
    let result = load_weight (cstr name) count 0 0;
    if result == int64_from_int 0 then None else Some result
  ;;
  let free = fn buffer -> let _ = free_weight buffer; Some GpuDone;;
;
let Linear = module () ->
  let load = fn model: (QwenModel) name: (String) inputs outputs ->
    let result = load_weight (cstr name) inputs outputs 1;
    if result == int64_from_int 0 then None
    else Some (as_projection (result, int64_from_int 0, inputs, outputs))
  ;;
  let load_unbiased = fn model: (QwenModel) name: (String) inputs outputs ->
    let result = load_weight (cstr name) inputs outputs 2;
    if result == int64_from_int 0 then None
    else Some (as_projection (result, int64_from_int 0, inputs, outputs))
  ;;
  let free = fn layer: (Projection) -> Gpu.free layer.weight;;
;
'''
suffix = '''
let model = (Ptr 1, 8, Ptr 1, 0, (Ptr 1, 2, 2, Ptr 1), [| Ptr 1 |], [| 2 |], [| 2 |]);
let result = OPERATION;
let present = match result with
| None -> 0
| Some layer -> if layer.norm == int64_from_int 1 && layer.q.weight == int64_from_int 2
    && layer.k.weight == int64_from_int 3 && layer.v.weight == int64_from_int 4
    && layer.o.weight == int64_from_int 5 && layer.post == int64_from_int 6
    && layer.gate.weight == int64_from_int 7 && layer.up.weight == int64_from_int 8
    && layer.down.weight == int64_from_int 9 then 1 else 2;
print `RESULT {present} {call_count ()} {free_count ()} {correct ()}\\n`;
'''
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    library = root / 'stubs.so'
    (root / 'stubs.c').write_text(stubs)
    subprocess.run(['cc', '-shared', '-fPIC', str(root / 'stubs.c'), '-o', str(library)], check=True)
    cases = [(failure, 'Decoder.load model 2 8', int(failure == 0), failure or 9, max(0, failure - 1))
             for failure in range(10)]
    cases += [(0, 'DecoderLoad.start model 2 8 |> DecoderLoad.finish', 0, 0, 0),
              (0, 'DecoderLoad.start model 2 8 |> DecoderLoad.step InputNorm |> DecoderLoad.step Query |> DecoderLoad.finish', 0, 2, 2)]
    for failure, operation, present, calls, freed in cases:
        script = root / 'decoder.ylc'
        script.write_text(f'let _ = dlopen "{library}";\n' + model_types + projection_types +
                          prefix.replace('FAILURE', str(failure)) + loader + suffix.replace('OPERATION', operation))
        result = subprocess.run(['build/ylc', str(script)], capture_output=True, text=True, timeout=30)
        expected = f'RESULT {present} {calls} {freed} 1'
        assert result.returncode == 0 and expected in result.stdout, (failure, operation, expected, result.stdout, result.stderr)
    script.write_text(script.read_text() + '\nlet load_again = fn () -> Decoder.load model 2 8;;\n')
    session = 'configure 0;\nif load_again () |> (fn result -> match result with | Some layer -> true | None -> false) then print "LOADED\\n" else print "FAILED\\n";\n'
    result = subprocess.run(['build/ylc', '-i', str(script)], input=session * 2 + '%quit\n',
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 0 and result.stdout.count('LOADED\n') == 2, result.stdout + result.stderr
    print('Decoder loading passed: success, nine failures, incomplete builders, and REPL reuse')
PY
