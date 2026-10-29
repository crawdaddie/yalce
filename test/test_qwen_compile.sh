#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - "${1:-loadqwen.ylc}" <<'PY'
from pathlib import Path
import subprocess
import tempfile
import sys

source = Path(sys.argv[1]).read_text()
start = source.index('let Kernels = module () ->')
end = source.index('\n;\n', start) + len('\n;\n')
module = source[start:end]
stubs = r'''
#include <stddef.h>
#include <string.h>

enum Stage { CREATE = 1, COMPILE, LOG_SIZE, LOG, PTX_SIZE, PTX, LOAD, FIND };
enum { OK = 0, ERROR = 1 };
static int failed, calls, destroyed, unloaded;
static char program, module, function;
int configure(int stage) {
    failed = stage;
    calls = destroyed = unloaded = 0;
    return OK;
}
int called(void) { return calls; }
int destroys(void) { return destroyed; }
int unloads(void) { return unloaded; }
int correct_function(void *p) { return p == &function; }
static int status(int stage) {
    calls |= 1 << stage;
    return failed == stage ? ERROR : OK;
}
int nvrtcCreateProgram(void **out, const char *src, const char *name,
                       int count, const char **headers, const char **names) {
    *out = &program;
    return status(CREATE);
}
int nvrtcCompileProgram(void *p, int n, const char **options) {
    int result = status(COMPILE);
    return failed == LOG_SIZE || failed == LOG ? ERROR : result;
}
int nvrtcGetProgramLogSize(void *p, size_t *size) {
    *size = sizeof("compile error\n");
    return status(LOG_SIZE);
}
int nvrtcGetProgramLog(void *p, char *log) {
    strcpy(log, "compile error\n");
    return status(LOG);
}
int nvrtcGetPTXSize(void *p, size_t *size) {
    *size = sizeof("ptx");
    return status(PTX_SIZE);
}
int nvrtcGetPTX(void *p, char *ptx) {
    strcpy(ptx, "ptx");
    return status(PTX);
}
int cuModuleLoadData(void **out, const void *ptx) {
    *out = &module;
    return status(LOAD);
}
int cuModuleGetFunction(void **out, void *m, const char *name) {
    *out = &function;
    return status(FIND);
}
int nvrtcDestroyProgram(void **p) { ++destroyed; *p = NULL; return OK; }
int cuModuleUnload(void *m) { ++unloaded; return OK; }
'''
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    (root / 'stubs.c').write_text(stubs)
    library = root / 'stubs.so'
    subprocess.run(['cc', '-shared', '-fPIC', str(root / 'stubs.c'),
                    '-o', str(library)], check=True)
    module = module.replace('/opt/cuda/lib64/libnvrtc.so.13', str(library))
    prefix = '''open std/Opt;
let int64_from_int = extern fn Int -> Uint64;
let cuModuleLoadData = extern fn Ptr -> Ptr -> Int;
let cuModuleGetFunction = extern fn Ptr -> Ptr -> Ptr -> Int;
'''
    suffix = '''
let configure = extern fn Int -> Int;
let called = extern fn () -> Int;
let destroys = extern fn () -> Int;
let unloads = extern fn () -> Int;
let correct_function = extern fn Ptr -> Int;
let _ = configure FAILURE;
let again = fn name src -> Kernels.compile_kernel name src;;
let result = again "test" "source";
let present = match result with | Some function -> correct_function function | None -> 0;
print `RESULT {present} {called ()} {destroys ()} {unloads ()}\\n`;
'''
    # Each failed stage prevents later work and releases only acquired resources.
    stages = {
        0: [1, 2, 5, 6, 7, 8],
        1: [1], 2: [1, 2, 3, 4], 3: [1, 2, 3],
        4: [1, 2, 3, 4], 5: [1, 2, 5],
        6: [1, 2, 5, 6], 7: [1, 2, 5, 6, 7],
        8: [1, 2, 5, 6, 7, 8],
    }
    for failure, expected_stages in stages.items():
        script = root / 'compile.ylc'
        script.write_text(prefix + module + suffix.replace('FAILURE', str(failure)))
        result = subprocess.run(['build/ylc', str(script)],
                                capture_output=True, text=True, timeout=30)
        assert result.returncode == 0, result.stdout + result.stderr
        mask = sum(1 << stage for stage in expected_stages)
        expected = f'RESULT {int(failure == 0)} {mask} {int(failure != 1)} {int(failure == 8)}'
        assert expected in result.stdout, result.stdout + result.stderr
    # Reusing callbacks in later REPL chunks must preserve their type metadata.
    session = 'configure 0;\nagain "test" "source";\n%quit\n'
    result = subprocess.run(['build/ylc', '-i', str(script)], input=session,
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'Error' not in result.stderr, result.stderr
    print('Kernel compilation passed: success, eight failure paths, and REPL reuse')
PY
