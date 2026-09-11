# Audio JIT Local Synth Functions And HOF Plan

This plan covers functions defined inside an enclosing `@Audio` function and the
later higher-order forms that instantiate them repeatedly.

The key design point: a local audio function is still a synth function. The only
difference is scope. It should get its own `cons`, `init`, and `kernel`
functions, and calls from the enclosing synth should call that local kernel with
a state pointer owned by the enclosing synth.

## Goals

- Treat a local `fn` inside `@Audio` as a scoped audio function.
- Generate a local synth bundle:
  - `local.cons`
  - `local.init`
  - `local.kernel`
- Keep the local synth symbol visible only inside the enclosing audio scope.
- Compute the local synth state requirement independently.
- Instantiate local synth state inside the enclosing synth state.
- Emit local init calls from the enclosing init path.
- Emit local kernel calls from the enclosing kernel/frame path.
- Preserve multi-channel expansion by creating one local state instance per
  lane.
- Later, support static HOFs that instantiate callback synths once per
  iteration.

## Current Shape

The existing top-level/module audio symbol model is already close:

- `MirAudioSynthSymbol` points at a synth bundle.
- `audio_mir_emit_synth_in_audio_context` calls a synth kernel from another
  audio context.
- Stateful primitives reserve state using `AudioStateSlot`.
- Parent `state_cursor` determines the final node state size.
- Parent init zeroes state and emits literal initialization.

The missing piece is that local functions are currently not represented as real
audio synth bundles. They should be.

## Local Audio Function Model

For:

```ylc
let s = @Audio fn () ->
  let ap = fn freq q x ->
    Filter.allpass2 freq q x
  in
  saw_osc (~[40., 40.4]) |> ap 120. 4.
;;
```

Lower `ap` as a scoped synth bundle:

```text
s.ap.cons
s.ap.init
s.ap.kernel
```

The local symbol `ap` resolves only while lowering the body of `s`.

The local kernel has the same conceptual ABI as other audio kernels:

```text
ap.kernel(node, state, frame, spf, freq, q, x) -> sample
```

When `s.kernel` calls `ap`, it does not inline the body directly. It emits:

```text
ap.kernel(parent_node, ap_state_for_this_instance, frame, spf, freq, q, x_lane)
```

LLVM can still inline this later because it is a normal direct MIR/LLVM call.

## Local Bundle ABI

Use the same bundle structure as top-level audio functions, but local bundles do
not need a public engine-facing frame adapter unless they escape as nodes.

Required:

- `cons`: constructs or describes an embedded instance
- `init`: initializes a local state block
- `kernel`: computes one sample

Optional:

- `frame`: only needed if a local synth is allowed to escape and become a
  standalone `Node`. The first implementation should reject escaping local audio
  functions.

The important runtime callable from the enclosing synth is `kernel`.

## Constructor Meaning

Top-level `cons` creates a `Node`.

Local `cons` should be the embedded constructor path. It should not allocate a
new `Node`; it should describe or compute the local instance inside the parent.

Two viable implementations:

1. Generate an actual local `cons(parent_state, ...) -> Ptr` function that
   returns the substate pointer for this instance.
2. Keep `cons` as a lowering-time bundle entry and inline its effect into the
   parent constructor/init bookkeeping.

The second option is probably simpler initially. The local bundle still has a
`cons` concept, but applying it inside a parent performs:

- reserve `local.state_bytes` in the parent `AudioCompileCtx`
- record the substate offset
- record an init call to `local.init`
- return an `AudioSynthInstance` used by the parent kernel call

## Proposed Data Structures

Keep `MirAudioSynthSymbol`, but make it represent both exported and local synths:

```c
typedef enum AudioSynthScope {
  AUDIO_SYNTH_EXPORTED,
  AUDIO_SYNTH_LOCAL,
} AudioSynthScope;

typedef struct MirAudioSynthSymbol {
  const char *name;
  AudioSynthScope scope;
  int num_inputs;
  int state_bytes;
  MirFunction *ctor_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;
} MirAudioSynthSymbol;
```

Add an instance record for nested/local calls:

```c
typedef struct AudioSynthInstance {
  MirAudioSynthSymbol *synth;
  size_t state_offset;
  int lane;
  const char *name;
  struct AudioSynthInstance *next;
} AudioSynthInstance;
```

Add an init-call queue to the parent audio context:

```c
typedef struct AudioInitCall {
  MirFunction *init_fn;
  size_t state_offset;
  Type *state_ptr_type;
  const char *name;
  struct AudioInitCall *next;
} AudioInitCall;
```

Extend `AudioCompileCtx`:

```c
AudioSynthInstance *instances;
AudioInitCall *init_calls;
const char *state_prefix;
```

`state_prefix` is for readable/debuggable slot names only. State layout is still
determined by offsets.

## Binding Local Functions

In `audio_mir_let`, detect a lambda binding inside an audio context:

```ylc
let name = fn ... -> ...
```

or, later if syntax permits it:

```ylc
let name = @Audio fn ... -> ...
```

Instead of lowering this as a normal MIR closure:

1. Build a local audio synth bundle for the lambda.
2. Give it a scoped name like `<parent>.<local>`.
3. Compile its `kernel` in a child audio compile context.
4. Compile its `init` from the child state slots and init stores.
5. Create its `cons` entry.
6. Bind a `MIR_SYMBOL_CUSTOM` or `AudioLocalBinding` pointing to the
   `MirAudioSynthSymbol`.
7. Do not export it from the module.

The binding should shadow normal identifiers only inside the lexical body where
the `let` is visible.

## Captures

Local functions can capture values from the enclosing audio scope. Do not use a
runtime heap closure for the first version.

Represent captures as explicit hidden parameters on the local kernel:

```text
ap.kernel(node, state, frame, spf, <captures...>, <explicit args...>)
```

Capture categories:

- Audio values: passed to the local kernel at the call site as SSA values.
- Scalar MIR values: passed to the local kernel at the call site.
- Constants: either passed as hidden args or folded later.
- State-backed arrays from the parent: pass their array view/pointer as a hidden
  arg; do not duplicate their state unless the local function declares its own
  array.

For init:

- Only constants and local state declarations should affect `local.init`.
- Frame-varying captures are not available to `init`.
- If a captured value is needed for init and is not static, reject it initially.

## State Sizing

Compile each local bundle with its own state cursor first:

```text
local.state_bytes = local_audio_ctx.state_cursor
```

Then, every application of the local function inside the parent reserves a block
of that size in the parent:

```text
instance_offset = reserve(parent, local.state_bytes, local_align)
```

The parent kernel call passes:

```text
local_state = parent_state + instance_offset
```

This is cleaner than having local body lowering reserve directly in the parent.
The local function owns its internal layout; the parent only owns instances of
that layout.

## Init Emission

When an enclosing synth instantiates a local synth:

1. Reserve a state block in the parent.
2. Append an `AudioInitCall`:

```text
local.init(parent_state + instance_offset)
```

During parent init emission:

1. Zero parent state slots.
2. Emit literal stores for parent-owned arrays.
3. Emit queued local init calls.

This lets nested local synths initialize their own arrays, filters, envelopes,
and sub-synths without duplicating their init logic in the parent.

## Applying A Local Synth

In `AST_APPLICATION` inside `audio_mir_expr`:

1. Resolve `app->data.AST_APPLICATION.function`.
2. If it resolves to a scoped `MirAudioSynthSymbol`, use the same application
   path as top-level/module audio synth symbols.
3. Evaluate every flat argument in `app->data.AST_APPLICATION.args`.
4. Compute `lanes = max(argument lane counts)`.
5. For each lane:
   - reserve one parent state instance for this local synth
   - queue one local init call for that state
   - select/broadcast lane values for explicit args and captures
   - call the local kernel
6. Return one scalar `AudioValue` or a tuple `AudioValue`.

This is the same rule needed for top-level/module synth calls in audio context.
The only difference is symbol visibility and the embedded constructor behavior.

## Delay Feedback Loop Kernels

A delay feedback loop should not be represented as a cycle in the engine graph.
It should be an audio lowering primitive/HOF that owns the delay-line state and
calls a user-defined audio kernel once per sample.

The user-facing shape should be something close to:

```ylc
let s = @Audio fn x ->
  let fb = fn delayed input ->
    input + (delayed |> Filter.lpf 1200. 0.7) * 0.82
  in
  delay_feedback 48000 12000 fb x
;;
```

Conceptual type:

```text
delay_feedback : Int -> Int -> (Double -> Double -> Double) -> Double -> Double
```

Where:

- first `Int` is maximum delay-line length in samples and must be static
- second `Int` is delay time in samples and may be static initially
- callback receives `(delayed_sample, input_sample)`
- callback returns the value to write back into the delay line
- `delay_feedback` returns the delayed sample

This makes the feedback path explicit:

```text
delayed = delay_line[read_index]
write   = fb.kernel(node, fb_state, frame, spf, delayed, input)
delay_line[write_index] = write
return delayed
```

For more flexible effects, add a second form later where the callback returns a
tuple:

```text
delay_feedback2 : Int -> Int ->
  (Double -> Double -> (Double, Double)) -> Double -> Double
```

In that form the callback returns `(output_sample, write_sample)`.

### Delay State

The delay primitive owns its own state:

```c
typedef struct AudioDelayFeedbackState {
  uint64_t write_index;
  double line[];
} AudioDelayFeedbackState;
```

Because the line is variable sized, the MIR/audio lowering should not rely on
`sizeof(AudioDelayFeedbackState)`. Instead:

```text
state_bytes = align(sizeof(uint64_t), 8) + max_delay_samples * sizeof(double)
```

The emitted code computes:

```text
write_index_ptr = state + 0
line_ptr        = state + 8
read_index      = (write_index + max_delay - delay_samples) % max_delay
```

For the first version, require both `max_delay_samples` and `delay_samples` to
be compile-time integer constants. Dynamic/fractional delay can be added after
the integer version works.

### Callback Kernel

The callback must be a synth kernel callable, not a graph node:

```text
fb.kernel(node, fb_state, frame, spf, delayed, input) -> Double
```

If the callback is local:

```ylc
let fb = fn delayed input -> ...
```

then lowering creates:

```text
s.fb.cons
s.fb.init
s.fb.kernel
```

`delay_feedback` then instantiates `s.fb` inside the enclosing synth state,
exactly like any other nested local synth call.

State layout for one mono instance:

```text
s.state:
  delay_feedback.write_index
  delay_feedback.line[max_delay]
  fb.state
```

For multi-channel input, each lane gets its own delay-line state and its own
callback state:

```text
delay_feedback.lane0.state
delay_feedback.lane0.fb.state
delay_feedback.lane1.state
delay_feedback.lane1.fb.state
```

### Lowering Order

Per lane, emit this order in the enclosing kernel:

1. Load `write_index`.
2. Compute `read_index`.
3. Load `delayed`.
4. Evaluate/broadcast the current input lane.
5. Call the callback kernel with `delayed` and input.
6. Store callback result to `line[write_index]`.
7. Increment and wrap `write_index`.
8. Return `delayed` or the callback-provided output.

This strict ordering is the reason it should not be a graph cycle. The feedback
sample for frame `n` must be read before frame `n` writes the next value.

### Delay Feedback Apply Helper

Add a specialized helper:

```c
static AudioValue audio_mir_emit_delay_feedback(
    AudioCompileCtx *audio,
    Ast *origin,
    AudioValue max_delay_samples,
    AudioValue delay_samples,
    AudioValue callback,
    AudioValue input);
```

This helper should:

- statically evaluate `max_delay_samples`
- initially statically evaluate `delay_samples`
- normalize input lanes
- instantiate delay-line state per lane
- instantiate callback synth state per lane
- emit the read/callback/write sequence
- return one scalar value or a tuple of lane outputs

Later, refactor it through the same callable machinery used by `map`, `fold`,
and `foldi`.

### Fractional Delay Later

After integer delay works, add:

```text
delay_feedback_frac : Int -> Double ->
  (Double -> Double -> Double) -> Double -> Double
```

This uses linear interpolation:

```text
read_pos = write_index - delay_samples
i0 = floor(read_pos)
i1 = i0 + 1
frac = read_pos - floor(read_pos)
delayed = lerp(line[i0], line[i1], frac)
```

For modulated delay, interpolation should read before writing, and the write
index should still advance by exactly one sample per frame.

## Partial Application

Partial application of local audio functions should produce a partial audio
symbol, not a normal MIR closure.

Representation:

```c
typedef struct AudioPartialSynth {
  MirAudioSynthSymbol *synth;
  AudioValue *captured_args;
  size_t captured_argc;
} AudioPartialSynth;
```

Application combines captured args with the flat new args and then uses the same
lane-aware synth application path.

Escaping a partial local synth outside the enclosing `@Audio` context should be
rejected initially.

## Higher-Order Forms

The HOFs should be static unrollers in audio lowering:

- `map`
- `fold`
- `foldi`

The collection/range input must be compile-time constant for the first version.

Callback values may be:

- local audio synth symbols
- module/top-level audio synth symbols
- partial audio synths
- audio builtins/partial builtins
- plain MIR functions only if all arguments/results are non-stateful MIR values

## Static Domain Evaluation

Add a small compile-time evaluator:

```c
typedef enum AudioStaticValueKind {
  AUDIO_STATIC_NONE,
  AUDIO_STATIC_INT,
  AUDIO_STATIC_DOUBLE,
  AUDIO_STATIC_LIST,
  AUDIO_STATIC_ARRAY_LITERAL,
} AudioStaticValueKind;
```

Supported initially:

- integer literals
- double literals
- list literals with static elements
- array literals with static elements

Unsupported dynamic domains should produce explicit audio lowering errors.

## map

For:

```ylc
map f [a, b, c]
```

Lower as:

```ylc
[f a, f b, f c]
```

If `f` is a synth function, each element creates a separate synth instance.

For stereo/multi-lane inputs, each element/lane pair gets a separate state
instance:

```text
map.callsite.0.lane0
map.callsite.0.lane1
map.callsite.1.lane0
map.callsite.1.lane1
...
```

## fold

For:

```ylc
fold f init [a, b, c]
```

Lower as:

```ylc
acc0 = init
acc1 = f acc0 a
acc2 = f acc1 b
acc3 = f acc2 c
acc3
```

If `f` is a synth function, this creates one synth instance per iteration and
per lane.

## foldi

For:

```ylc
foldi n init f
```

Require `n` to be a static integer. Lower as:

```ylc
acc = init
for i in 0..n:
  acc = f acc i
```

Each iteration gets a distinct callback synth instance. The integer `i` should
be emitted as a MIR constant and cast to the callback formal type if needed.

## Shared Apply Helper

Add one helper that all normal calls and HOFs use:

```c
static AudioValue audio_mir_apply_audio_callable(
    AudioCompileCtx *audio,
    Ast *origin,
    MirAudioSynthSymbol *synth,
    AudioValue *captures,
    size_t capture_count,
    AudioValue *args,
    size_t arg_count,
    const char *instance_prefix);
```

Responsibilities:

- compute lane count
- reserve one instance state block per lane
- queue init calls
- pass captures and explicit args to the kernel
- construct tuple output for multi-lane results

This same helper should be used by:

- ordinary local synth calls
- ordinary top-level/module synth calls inside audio context
- partial synth calls
- `map`
- `fold`
- `foldi`

## Errors To Add

Reject clearly:

- local audio function escapes enclosing audio scope
- recursive local audio function
- HOF domain is not static
- HOF callback is not an audio-callable value
- init depends on frame-varying captures
- callback returns an unsupported shape

## Implementation Order

1. Split synth bundle metadata from symbol binding so bundles can be scoped.
2. Add local synth symbol binding for audio `let name = fn ...`.
3. Generate local `cons`, `init`, and `kernel` functions.
4. Make local bundle compilation use its own state cursor.
5. Add parent `AudioInitCall` queue.
6. Change nested synth application to reserve parent instance state and queue
   init calls.
7. Add hidden capture parameters to local kernels.
8. Add partial local/top-level synth application as an audio value.
9. Refactor normal synth calls and HOF calls through
   `audio_mir_apply_audio_callable`.
10. Add static domain evaluator.
11. Add `foldi`.
12. Add `fold`.
13. Add `map`.

## Expected Example

Source:

```ylc
let s = @Audio fn () ->
  let ap = fn freq q x ->
    Filter.allpass2 freq q x
  in
  saw_osc (~[40., 40.4]) |> ap 120. 4.
;;
```

Expected MIR shape:

```text
fn s.ap.cons(...)
fn s.ap.init(state: Ptr) -> ()
fn s.ap.kernel(node, state, frame, spf, freq, q, x) -> Double

fn s.kernel(node, state, frame, spf) -> (Double, Double)
  saw_l = saw_kernel(...)
  saw_r = saw_kernel(...)

  ap_l_state = state + offset_ap_lane_0
  ap_l = s.ap.kernel(node, ap_l_state, frame, spf, 120., 4., saw_l)

  ap_r_state = state + offset_ap_lane_1
  ap_r = s.ap.kernel(node, ap_r_state, frame, spf, 120., 4., saw_r)

  return (ap_l, ap_r)

fn s.init(state)
  s.ap.init(state + offset_ap_lane_0)
  s.ap.init(state + offset_ap_lane_1)
```

Expected parent constructor:

```text
s.cons creates one Node with output_lanes = 2 and state_bytes including:
- saw lane 0 state
- saw lane 1 state
- ap lane 0 state block
- ap lane 1 state block
```

The local audio function is therefore not just inlined syntax. It is a scoped
synth bundle whose kernel is directly callable from the enclosing synth.
