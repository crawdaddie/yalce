#!/usr/bin/env bash
# REPL persistence tests.
#
# Each case pipes a multi-line session into the native ylc REPL and asserts
# that bindings made in earlier inputs (value globals, functions, generic
# functions) are visible to later inputs. The top-level MIR scope must
# persist across compile calls for this to work; before that wiring
# existed, input N+1 errored with "unresolved identifier" for any name
# bound in input N.
#
# Run:  bash test/test_repl_persistence.sh
#        (after `make` from the repo root)

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
YLC="$ROOT_DIR/build/ylc"

if [ ! -x "$YLC" ]; then
  echo "error: $YLC does not exist; run make from the repository root first" >&2
  exit 1
fi

if [ "${REPL_TEST_COLOR:-1}" = "0" ]; then
  GREEN=''; RED=''; BLUE=''; BOLD=''; NC=''
else
  GREEN=$'\033[0;32m'
  RED=$'\033[0;31m'
  BLUE=$'\033[0;34m'
  BOLD=$'\033[1m'
  NC=$'\033[0m'
fi

CHECKS=0
PASSES=0
FAILS=0

strip_ansi() {
  sed -E $'s/\x1b\\[[0-9;]*[A-Za-z]//g'
}

pass() {
  CHECKS=$((CHECKS + 1)); PASSES=$((PASSES + 1))
  echo "  ${GREEN}✓${NC} $1"
}

fail() {
  CHECKS=$((CHECKS + 1)); FAILS=$((FAILS + 1))
  echo "  ${RED}✗${NC} $1"
}

# Run a piped REPL session and return the stripped, prompt-stripped transcript.
# Inputs are newline-separated YLC statements (each becomes one REPL input).
run_repl() {
  local input="$1"
  printf '%s\n' "$input" | "$YLC" 2>&1 | strip_ansi
}

run_repl_continued() {
  local input="$1"
  printf '%s\n' "$input" '' '%quit' | "$YLC" 2>&1 | strip_ansi
}

# Assert the transcript contains a pattern (case passes) — used for
# successful cross-input references that print a value.
# Assert the transcript does NOT contain a pattern — used to catch the
# "unresolved identifier" error that indicates cross-input visibility
# regressed.
assert_not_contains() {
  local transcript="$1"
  local pattern="$2"
  local label="$3"
  if printf '%s' "$transcript" | grep -Eq "$pattern"; then
    fail "$label (unexpected pattern found: $pattern)"
    printf '%s\n' "$transcript" | sed 's/^/      | /' >&2
  else
    pass "$label"
  fi
}

assert_contains() {
  local transcript="$1"
  local pattern="$2"
  local label="$3"
  if printf '%s' "$transcript" | grep -Eq "$pattern"; then
    pass "$label"
  else
    fail "$label (missing pattern: $pattern)"
    printf '%s\n' "$transcript" | sed 's/^/      | /' >&2
  fi
}

section() {
  echo
  echo "${BLUE}${BOLD}==>${NC} ${BOLD}$1${NC}"
}

# ---------------------------------------------------------------------------

section "value globals persist across REPL inputs"

# let x = 10; then y = x + 5 must resolve x from the prior input. The
# native REPL does not echo top-level values, so the assertion is the
# absence of any error ("unresolved identifier" / "Error") — that's the
# only signal the REPL surfaces for a cross-input reference.
OUT=$(run_repl 'let x = 10
let y = x + 5')
assert_not_contains "$OUT" 'unresolved identifier' "second input sees prior value binding"
assert_not_contains "$OUT" 'Error' "cross-input value reference typechecks and lowers"

# ---------------------------------------------------------------------------

section "functions persist across REPL inputs"

# Define a function, then call it in the next input. `fn` bodies need
# `;;` to terminate the statement.
OUT=$(run_repl 'let inc = fn n -> n + 1;;
inc 41')
assert_not_contains "$OUT" 'unresolved identifier' "second input sees prior function binding"
assert_not_contains "$OUT" 'Error' "cross-input function call typechecks and lowers"

# Recursive function defined in one input, called in a later one. Plain
# `let` (no `rec` keyword) — the parser resolves self-reference within
# the binding.
OUT=$(run_repl 'let fact = fn n -> match n with | 0 -> 1 | _ -> n * (fact (n - 1));;
fact 5')
assert_not_contains "$OUT" 'unresolved identifier' "recursive function visible across inputs"
assert_not_contains "$OUT" 'Error' "recursive cross-input call typechecks and lowers"

# ---------------------------------------------------------------------------

section "generic functions specialize across REPL inputs"

# A polymorphic identity defined in input 1, applied at Int in input 2.
OUT=$(run_repl 'let id = fn x -> x;;
id 7')
assert_not_contains "$OUT" 'unresolved identifier' "generic function visible across inputs"
assert_not_contains "$OUT" 'Error' "generic id applied at Int typechecks and lowers"

# Same id applied at a different type in a third input — exercises
# specialization reuse across the persistent scope.
OUT=$(run_repl 'let id2 = fn x -> x;;
id2 3
id2 true')
assert_not_contains "$OUT" 'unresolved identifier' "generic function reusable across multiple inputs"
assert_not_contains "$OUT" 'Error' "generic id applied at Bool typechecks and lowers"

# ---------------------------------------------------------------------------

section "multi-line inputs via backslash continuation"

# The REPL joins a line ending in `\` with the next line into a single
# input. This is how multi-line `fn`/`match` forms are entered at the
# prompt. Piped stdin expresses it the same way: each `\`-terminated line
# is one physical line, the continuation reads the next line.
OUT=$(run_repl 'let g = fn x ->\
  x + 1;;
g 9')
assert_not_contains "$OUT" 'syntax error' "backslash-continued fn body parses as one input"
assert_not_contains "$OUT" 'unresolved identifier' "multi-line-defined function visible to next input"
assert_not_contains "$OUT" 'Error' "multi-line fn called in a later input"

# Three-line match expression entered as one REPL input via continuation.
OUT=$(run_repl 'let h = fn n ->\
  match n with\
  | 0 -> 100\
  | _ -> 200;;
h 0')
assert_not_contains "$OUT" 'syntax error' "three-line match continuation parses as one input"
assert_not_contains "$OUT" 'unresolved identifier' "multi-line match function visible to next input"
assert_not_contains "$OUT" 'Error' "multi-line match function evaluated"

# Continuation must NOT bleed into the next input: a `\`-terminated
# multi-line input followed by a separate single-line input should treat
# them as two REPL inputs, not one joined blob. Uses a fn body (which
# legitimately spans lines) so the continuation is well-formed.
OUT=$(run_repl 'let k = fn x ->\
  x * 2;;
let m = k 21')
assert_not_contains "$OUT" 'syntax error' "continuation input and next input stay separate"
assert_not_contains "$OUT" 'unresolved identifier' "continuation-defined function visible to next input"
assert_not_contains "$OUT" 'Error' "continuation-defined function usable in next input"

# Blank line between inputs is ignored (readline returns "" for an empty
# line; the REPL skips it rather than treating it as a parse error).
OUT=$(run_repl 'let r = 5

let s = r + 1')
assert_not_contains "$OUT" 'syntax error' "blank line between inputs does not break parsing"
assert_not_contains "$OUT" 'unresolved identifier' "value after blank-line-separated input resolves"
assert_not_contains "$OUT" 'Error' "blank-line-separated cross-input reference typechecks"

# ---------------------------------------------------------------------------

section "bindings accumulate over many inputs"

# Three successive value bindings, each referencing the prior one.
OUT=$(run_repl 'let a = 1
let b = a + 1
let c = b + 1
let d = c + 1')
assert_not_contains "$OUT" 'unresolved identifier' "chain of 4 inputs resolves each prior binding"
assert_not_contains "$OUT" 'Error' "a->b->c->d chain typechecks and lowers"

# ---------------------------------------------------------------------------

section "same-input functions read globals"

OUT=$(run_repl_continued 'let arr = array_fill_const 1 0; \
let f = fn () -> arr[0];; \
f ();')
assert_not_contains "$OUT" 'could not resolve call callee' "same-input function reads array global"
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "same-input global function call runs"

OUT=$(run_repl_continued 'let _ = dlopen "./engine/build/libyalce_synth.so"; \
let cancel_task = extern fn Ptr -> (); \
let task_ref = array_fill_const 1 (Ptr 0); \
let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \
task (Ptr 0);')
assert_not_contains "$OUT" 'cancel_task\.[0-9]+' "interactive extern keeps C symbol name"
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "same-input extern global function call runs"

OUT=$(printf '%s\n' \
  'let _ = dlopen "./engine/build/libyalce_synth.so"; \' \
  'let cancel_task = extern fn Ptr -> (); \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'task (Ptr 0);' \
  '' \
  'task (Ptr 0);' \
  '%quit' | "$YLC" 2>&1 | strip_ansi)
assert_not_contains "$OUT" 'duplicate definition of symbol' "second task stop does not redefine drop helpers"
assert_not_contains "$OUT" 'double free|corruption|Segmentation fault|SIGSEGV|JIT session error|Error' "second task stop does not crash"

OUT=$(printf '%s\n' \
  'let _ = dlopen "./libs/audio_jit/libaudio_jit.so"; \' \
  'type Task = Ptr; \' \
  'let play_pattern = extern fn Double -> T -> Task; \' \
  'let cancel_task = extern fn Task -> (); \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let co = fn () -> yield 0.01; yield co ();; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'task (Ptr 0);' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1 | strip_ansi)
assert_not_contains "$OUT" 'free\(\): invalid size|invalid pointer|double free|corruption|corrupted size' "task stop keeps allocator state valid"
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "task stop after play_pattern runs"

OUT=$(printf '%s\n' \
  'let _ = dlopen "./libs/audio_jit/libaudio_jit.so"; \' \
  'open libs/audio_jit/DSP_common; \' \
  'let scheduler_event_loop = extern fn () -> Int; \' \
  'let usleep = extern fn Int -> Int; \' \
  'scheduler_event_loop (); \' \
  'type Task = Ptr; \' \
  'let play_pattern = extern fn Double -> T -> Task; \' \
  'let cancel_task = extern fn Task -> (); \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let child = fn () -> yield 1.; yield child ();; \' \
  'let co = fn () -> child () |> play_pattern 0.; yield 4.; yield co ();; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'usleep 50000; task (Ptr 0);' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1)
STATUS=$?
OUT=$(printf '%s' "$OUT" | strip_ansi)
if [ "$STATUS" -eq 0 ]; then
  pass "task stop after child play_pattern exits cleanly"
else
  fail "task stop after child play_pattern exits cleanly (status $STATUS)"
  printf '%s\n' "$OUT" | sed 's/^/      | /' >&2
fi
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "task stop after child play_pattern runs"

OUT=$(printf '%s\n' \
  'let _ = dlopen "./libs/audio_jit/libaudio_jit.so"; \' \
  'open libs/audio_jit/DSP_common; \' \
  'let scheduler_event_loop = extern fn () -> Int; \' \
  'let usleep = extern fn Int -> Int; \' \
  'scheduler_event_loop (); \' \
  'type Task = Ptr; \' \
  'let play_pattern = extern fn Double -> T -> Task; \' \
  'let cancel_task = extern fn Task -> (); \' \
  'let pass = fn p -> p;; \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let xs = [| 1., 1. |]; \' \
  'let co = fn () -> xs |> iter |> cor_map (fn d -> pass (Ptr 0); d) |> play_pattern 0.; yield 4.; yield co ();; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'usleep 50000; task (Ptr 0);' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1)
STATUS=$?
OUT=$(printf '%s' "$OUT" | strip_ansi)
if [ "$STATUS" -eq 0 ]; then
  pass "task stop after mapped child pattern exits cleanly"
else
  fail "task stop after mapped child pattern exits cleanly (status $STATUS)"
  printf '%s\n' "$OUT" | sed 's/^/      | /' >&2
fi
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "task stop after mapped child pattern runs"

OUT=$(printf '%s\n' \
  'let _ = dlopen "./libs/audio_jit/libaudio_jit.so"; \' \
  'open libs/audio_jit/DSP_common; \' \
  'let scheduler_event_loop = extern fn () -> Int; \' \
  'let usleep = extern fn Int -> Int; \' \
  'scheduler_event_loop (); \' \
  'type Task = Ptr; \' \
  'let play_pattern = extern fn Double -> T -> Task; \' \
  'let cancel_task = extern fn Task -> (); \' \
  'let play_node = fn p -> p;; \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let fx = @Audio fn x -> x |> Filter.lpf 1000. 0.2;; \' \
  'let xs = [| 1., 1. |]; \' \
  'let co = fn () -> xs |> iter |> cor_map (fn d -> fx d |> play_node; d) |> play_pattern 0.; yield 4.; yield co ();; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'usleep 50000; task (Ptr 0);' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1)
STATUS=$?
OUT=$(printf '%s' "$OUT" | strip_ansi)
if [ "$STATUS" -eq 0 ]; then
  pass "task stop after imported audio kernel exits cleanly"
else
  fail "task stop after imported audio kernel exits cleanly (status $STATUS)"
  printf '%s\n' "$OUT" | sed 's/^/      | /' >&2
fi
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "task stop after imported audio kernel runs"

OUT=$(printf '%s\n' \
  'open test/fixtures/audio_jit/DSP_noinit; \' \
  'let scheduler_event_loop = extern fn () -> Int; \' \
  'let usleep = extern fn Int -> Int; \' \
  'scheduler_event_loop (); \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let fx = @Audio fn x -> x |> Filter.lpf 1000. 0.2;; \' \
  'let xs = [| 1., 1. |]; \' \
  'let co = fn () -> xs |> iter |> cor_map (fn d -> fx d |> play_node; d) |> play_pattern 0.; yield 4.; yield co ();; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'usleep 50000; co () |> play_pattern 0. |> task;' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1)
STATUS=$?
OUT=$(printf '%s' "$OUT" | strip_ansi)
if [ "$STATUS" -eq 0 ]; then
  pass "second play_pattern start exits cleanly"
else
  fail "second play_pattern start exits cleanly (status $STATUS)"
  printf '%s\n' "$OUT" | sed 's/^/      | /' >&2
fi
assert_not_contains "$OUT" 'Symbols not found: \[ play_pattern \]' "second play_pattern start keeps audio_jit builtin"
assert_not_contains "$OUT" 'duplicate definition of symbol' "second play_pattern start does not duplicate module init"
assert_not_contains "$OUT" 'corrupted size|Segmentation fault|SIGSEGV|JIT session error|Error' "second play_pattern start does not crash"

OUT=$(printf '%s\n' \
  'open test/fixtures/audio_jit/DSP_noinit; \' \
  'let tempo_mul = fn () -> 60. / 150.;; \' \
  'let scheduler_event_loop = extern fn () -> Int; \' \
  'let usleep = extern fn Int -> Int; \' \
  'scheduler_event_loop (); \' \
  'import std/Math; \' \
  'let task_ref = array_fill_const 1 (Ptr 0); \' \
  'let task = fn t -> task_ref[0] |> cancel_task; task_ref[0] := t;; \' \
  'let fx = @Audio fn lo dr -> lo;; \' \
  'let xs = [| 0.75, 0.5, 0.25 |]; \' \
  'let co = fn () -> \' \
  '  xs |> iter |> cor_map (fn d -> fx (Math.rrand 200. 400.) 0.01 |> play_node; d * tempo_mul ()) |> play_pattern 0.; \' \
  '  yield 4. * tempo_mul (); \' \
  '  yield co () \' \
  ';; \' \
  'co () |> play_pattern 0. |> task;' \
  '' \
  'usleep 50000; co () |> play_pattern 0. |> task;' \
  'usleep 50000; task (Ptr 0);' \
  '%quit' | "$YLC" -i --base "$ROOT_DIR" 2>&1)
STATUS=$?
OUT=$(printf '%s' "$OUT" | strip_ansi)
if [ "$STATUS" -eq 0 ]; then
  pass "imported rrand task replacement exits cleanly"
else
  fail "imported rrand task replacement exits cleanly (status $STATUS)"
  printf '%s\n' "$OUT" | sed 's/^/      | /' >&2
fi
assert_not_contains "$OUT" 'corrupted size|Segmentation fault|SIGSEGV|JIT session error|Error' "imported rrand task replacement does not crash"

# ---------------------------------------------------------------------------

section "module imports persist across REPL inputs"

OUT=$(printf '%s\n' 'import std/Math' 'Math.rand_double ()' |
  "$YLC" --base "$ROOT_DIR" --dump-ir 2>&1 | strip_ansi)
assert_contains "$OUT" 'tail call double @rand_double\(\)' "nullary imported extern call returns Double in LLVM IR"
assert_contains "$OUT" 'declare double @rand_double\(\)' "nullary imported extern declaration uses Double return"
assert_not_contains "$OUT" 'declare ptr @rand_double\(\)' "nullary imported extern is not lowered as a pointer return"
assert_not_contains "$OUT" 'Error|JIT session error' "Math.rand_double call typechecks and lowers"

OUT=$(printf '%s\n' 'open std/Math' 'rand_int 200' 'rand_double ()' 'abs -7' '%quit' |
  "$YLC" -i --base "$ROOT_DIR" 2>&1 | strip_ansi)
assert_contains "$OUT" 'Int: [0-9]+' "opened imported extern is visible in next REPL input"
assert_contains "$OUT" 'Double: [0-9.-]+' "opened nullary Double extern runs in next REPL input"
assert_contains "$OUT" 'Int: 7' "opened module-defined function runs in next REPL input"
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "open std/Math persists without crashing"

SCRIPT=$(mktemp "${TMPDIR:-/tmp}/ylc-open-math.XXXXXX.ylc")
printf '%s\n' 'open std/Math;' 'print `{ ' 'rand_double ()' '}`' > "$SCRIPT"
OUT=$("$YLC" --base "$ROOT_DIR" "$SCRIPT" 2>&1 | strip_ansi)
rm -f "$SCRIPT"
assert_contains "$OUT" '[0-9]+\.[0-9]+' "open std/Math works in normal scripts"
assert_not_contains "$OUT" 'Segmentation fault|SIGSEGV|JIT session error|Error' "normal-script open std/Math does not crash"

# ---------------------------------------------------------------------------

echo
if [ "$FAILS" -eq 0 ]; then
  echo "${GREEN}${BOLD}✓ $PASSES/$CHECKS REPL persistence checks passed${NC}"
  exit 0
else
  echo "${RED}${BOLD}✗ $FAILS/$CHECKS REPL persistence checks failed${NC}"
  exit 1
fi
