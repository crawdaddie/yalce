#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

source = Path('loadqwen.ylc').read_text()
start = source.index('let as_device =')
end = source.index('\nlet upload_embedding', start)
gpu = source[start:end]
types = source[source.index('type MappedTensor ='):source.index('let as_qwen =')]
types += source[source.index('type Precision ='):source.index('# Driver helpers')]
externs = source[source.index('let cuInit ='):source.index('let Kernels =')]
stubs = r'''
#include <stdint.h>
#include <string.h>
enum Stage { INIT = 1, DEVICE, RETAIN, CURRENT, ALLOC, UPLOAD, COPY, READ, LAUNCH, SYNC, FREE };
enum { OK = 0, ERROR = 1, DEVICE_ID = 7, BUFFER = 64 };
static int failed, mask, frees, releases;
int configure(int stage) { failed = stage; mask = frees = releases = 0; return OK; }
int calls(void) { return mask; }
int freed(void) { return frees; }
int released(void) { return releases; }
static int status(int stage) { mask |= 1 << stage; return failed == stage ? ERROR : OK; }
int cuInit(unsigned flags) { return status(INIT); }
int cuDeviceGet(int *out, int index) { *out = DEVICE_ID; return status(DEVICE); }
int cuDevicePrimaryCtxRetain(void **out, int device) { *out = (void *)1; return status(RETAIN); }
int cuCtxSetCurrent(void *context) { return status(CURRENT); }
int cuDevicePrimaryCtxRelease(int device) { ++releases; return OK; }
int cuMemAlloc_v2(uint64_t *out, size_t bytes) { *out = BUFFER; return status(ALLOC); }
int cuMemFree_v2(uint64_t value) { ++frees; return status(FREE); }
int cuMemcpyHtoD_v2(uint64_t out, void *data, size_t bytes) { return status(UPLOAD); }
int cuMemcpyDtoD_v2(uint64_t out, uint64_t input, size_t bytes) { return status(COPY); }
int cuMemcpyDtoH_v2(void *out, uint64_t input, size_t bytes) { memset(out, 0, bytes); return status(READ); }
int cuLaunchKernel(void *f, unsigned x, unsigned y, unsigned z, unsigned bx, unsigned by,
                   unsigned bz, unsigned shared, void *stream, void *args, void *extra) { return status(LAUNCH); }
int cuCtxSynchronize(void) { return status(SYNC); }
'''
prefix = '''open std/Opt;
let int64_from_int = extern fn Int -> Uint64;
let configure = extern fn Int -> Int;
let calls = extern fn () -> Int;
let freed = extern fn () -> Int;
let released = extern fn () -> Int;
let _ = configure FAILURE;
let layer_source = "source";
let Kernels = module () ->
  let compile_kernel = fn name: (String) source: (String) -> Some (Ptr 1);;
;
let Json = module () ->
  let get = fn name: (String) header: (Ptr) -> Some header;;
;
let map_tensor = fn file: (Ptr) offset: (Int) object: (Ptr) ->
  Some (file, 2, 2, object)
;;
'''
suffix = '''
open Gpu;
let value = as_calculation (int64_from_int 8, 1, 2, F32);
let model = (Ptr 1, 8, Ptr 1, 0, (Ptr 1, 2, 2, Ptr 1), [| Ptr 1 |], [| 2 |], [| 2 |]);
let result = OPERATION;
let present = match result with | Some value -> 1 | None -> 0;
print `RESULT {present} {calls ()} {freed ()} {released ()}\\n`;
'''
# Each failure must stop dependent calls and release acquired resources.
init = [1, 2, 3, 4]
cases = []
for failure in range(1, 5):
    cases.append((failure, 'Gpu.alloc 8', 0, list(range(1, failure + 1)), 0, int(failure == 4)))
for failure in [0, 5, 7]:
    cases.append((failure, 'Gpu.clone value', int(failure == 0), init + [5] + ([7] if failure != 5 else []) + ([11] if failure == 7 else []), int(failure == 7), 0))
for failure in [0, 9, 10]:
    cases.append((failure, 'Gpu.launch (Ptr 1) 1 1 [| Ptr 0 |]', int(failure == 0), init + [9] + ([10] if failure != 9 else []), 0, 0))
for failure in [0, 8]:
    cases.append((failure, 'Gpu.download value', int(failure == 0), init + [8], 0, 0))
for failure in [0, 5, 6]:
    cases.append((failure, 'Gpu.weight model "weight" 2', int(failure == 0), init + [5] + ([6] if failure != 5 else []) + ([11] if failure == 6 else []), int(failure == 6), 0))
for failure in [0, 6, 11]:
    cases.append((failure, 'Gpu.temporary 8 (fn buffer: (Uint64) -> Gpu.upload buffer (Ptr 1) 8)', int(failure == 0), init + [5, 6, 11], 1, 0))
cases.append((0, 'Gpu.weight model "weight" 3', 0, init, 0, 0))
cases.append((1, 'Gpu.compile "kernel"', 0, [1], 0, 0))
cases.append((0, 'Gpu.compile "kernel"', 1, init, 0, 0))
# Owned binds preserve successful weights and unwind all earlier acquisitions.
cases.append((0, '(Gpu.alloc 8, Gpu.free) >>! (fn buffer -> Some buffer)', 1, init + [5], 0, 0))
cases.append((5, '(Gpu.alloc 8, Gpu.free) >>! (fn buffer -> Gpu.upload buffer (Ptr 1) 8)', 0, init + [5], 0, 0))
cases.append((6, '(Gpu.alloc 8, Gpu.free) >>! (fn first -> (Gpu.alloc 8, Gpu.free) >>! (fn second -> Gpu.upload second (Ptr 1) 8))', 0, init + [5, 6, 11], 2, 0))
cases.append((0, '(Gpu.alloc 8, Gpu.free) >>! (fn first -> (Gpu.alloc 8, Gpu.free) >>! (fn second -> Some (first, second)))', 1, init + [5], 0, 0))
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    library = root / 'stubs.so'
    (root / 'stubs.c').write_text(stubs)
    subprocess.run(['cc', '-shared', '-fPIC', str(root / 'stubs.c'), '-o', str(library)], check=True)
    for failure, operation, present, stages, frees, releases in cases:
        script = root / 'gpu.ylc'
        script.write_text(f'let _ = dlopen "{library}";\n' + types + externs +
                          prefix.replace('FAILURE', str(failure)) + gpu +
                          suffix.replace('OPERATION', operation))
        result = subprocess.run(['build/ylc', str(script)], capture_output=True, text=True, timeout=30)
        expected = f'RESULT {present} {sum(1 << stage for stage in stages)} {frees} {releases}'
        assert result.returncode == 0 and expected in result.stdout, (operation, failure, expected, result.stdout, result.stderr)
    print(f'GPU Option chains passed: {len(cases)} success and failure cases')
PY
