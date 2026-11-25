#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import os
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    helper = directory / 'allocations.c'
    library = directory / 'allocations.so'
    # Count actual malloc calls, including allocations discarded by codegen.
    helper.write_text('''
#include <stddef.h>
#include "lang/runtime/ylc_stdlib.h"
extern void *__libc_malloc(size_t size);
extern void __libc_free(void *ptr);
typedef struct {
  int head;
  void *tail;
} IntNode;
enum { TRACKED_NODES = 256 };
static void *nodes[TRACKED_NODES];
static int node_count;
static int active;
static int allocations;
void *malloc(size_t size) {
  if (active) {
    allocations++;
  }
  void *ptr = __libc_malloc(size);
  if (active && size == sizeof(YlcRcHeader) + sizeof(IntNode)) {
    if (node_count < TRACKED_NODES) {
      nodes[node_count++] = ptr;
    }
  }
  return ptr;
}
void free(void *ptr) {
  if (active) {
    for (int i = 0; i < node_count; i++) {
      if (nodes[i] == ptr) {
        nodes[i] = NULL;
      }
    }
  }
  __libc_free(ptr);
}
void probe_begin(void) {
  allocations = 0;
  node_count = 0;
  active = 1;
}
int probe_stop(void) {
  active = 0;
  return allocations;
}
int probe_balance(void) {
  int live = 0;
  for (int i = 0; i < node_count; i++) {
    live += nodes[i] != NULL;
  }
  return live;
}
''')
    subprocess.run(['cc', '-shared', '-fPIC', '-I.', str(helper), '-o', str(library)],
                   check=True)
    source = r'''
import std/Lists;
let probe_begin = extern fn () -> ();
let probe_stop = extern fn () -> Int;
let probe_balance = extern fn () -> Int;
let string_add = extern fn String -> String -> String;
let build = fn n acc ->
  if n == 0 then acc else build (n - 1) (n::acc)
;;
let check_unique = fn () ->
  let input = build 64 [];
  probe_begin ();
  let result = Lists.rev input;
  let allocations = probe_stop ();
  let total = Lists.fold (fn sum x -> sum + x) 0 result;
  print `unique={allocations == 0 && total == 2080}
unique_allocations={allocations}
`
;;
check_unique ();
let check_shared = fn () ->
  let input = build 4 [];
  probe_begin ();
  let result = Lists.rev input;
  let allocations = probe_stop ();
  let original = match input with | 1::2::3::4::[] -> true | _ -> false;
  let reversed = match result with | 4::3::2::1::[] -> true | _ -> false;
  print `shared={original && reversed && allocations == 4}
`
;;
check_shared ();
let check_tail = fn () ->
  let tail = build 4 [];
  let input = 9::8::tail;
  probe_begin ();
  let result = Lists.rev input;
  let allocations = probe_stop ();
  let original = match tail with | 1::2::3::4::[] -> true | _ -> false;
  let reversed = match result with | 4::3::2::1::8::9::[] -> true | _ -> false;
  print `tail={original && reversed && allocations == 4}
`
;;
check_tail ();
let cleanup = fn () ->
  let input = build 64 [];
  let result = Lists.rev input;
  let original = match input with | 1::_ -> true | _ -> false;
  let reversed = match result with | 64::_ -> true | _ -> false;
  original && reversed
;;
probe_begin ();
let total = cleanup ();
let _ = probe_stop ();
let balance = probe_balance ();
print `cleanup={total && balance == 0}
cleanup_balance={balance}
`;
let check_refs = fn () ->
  let text = string_add "owned" " string";
  let result = Lists.rev [(text, [1, 2,]), (text, [3,]),];
  let valid = match result with
    | (a, 3::[])::(b, 1::2::[])::[] -> a == text && b == text
    | _ -> false;
  print `refs={valid && text == "owned string"}
`
;;
check_refs ();
let make_refs = fn () -> list_reverse [(string_add "escape" "d", [9,]),];;
let escaped = match make_refs () with
  | (text, 9::[])::[] -> text == "escaped"
  | _ -> false;
print `escaped={escaped}
`;
let rev_ints = fn values: (List of Int) -> Lists.rev values;;
let empty = rev_ints [];
let singleton = Lists.rev [7,];
let edge = match empty with
  | [] -> (match singleton with | 7::[] -> true | _ -> false)
  | _ -> false;
print `edge={edge}
`;
let long = Lists.rev (build 50000 []);
let count = Lists.fold (fn n _ -> n + 1) 0 long;
print `long={count == 50000}
`;
'''
    path = directory / 'reverse.ylc'
    path.write_text(source)
    environment = dict(os.environ, LD_PRELOAD=str(library))
    for optimization in ('-O0', '-O2'):
        result = subprocess.run(['./build/ylc', optimization, '--verify-ir',
                                 '--base', str(Path.cwd()), str(path)],
                                env=environment, text=True, capture_output=True,
                                timeout=60)
        output = result.stdout + result.stderr
        assert result.returncode == 0, output
        for name in ('unique', 'shared', 'tail', 'cleanup', 'refs', 'escaped', 'edge', 'long'):
            assert f'{name}=True\n' in output, f'{optimization}:\n{output}'
print('List reversal allocation and ownership checks passed at O0/O2')
PY
