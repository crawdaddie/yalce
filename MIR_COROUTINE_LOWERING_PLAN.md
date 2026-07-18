# MIR Coroutine Lowering Plan

Status: design plan only. No MIR coroutine implementation is described here as
already complete.

This plan maps the existing AST -> LLVM coroutine behavior onto the MIR pipeline.
It uses the current `lang/backend_llvm/coroutines/` implementation as evidence
for required behavior, but does not require MIR to preserve that exact ABI:

- coroutine constructors are functions that create an LLVM coroutine frame;
- `yield` stores a raw yielded `T` in the coroutine promise and suspends;
- resuming a coroutine returns `Option<T>`;
- nested `yield` of `Coroutine<T>` is flattened as "yield from";
- constructor arguments, coroutine closed-over values, and values live across
  suspend points must not stay in the caller stack frame.

The key MIR additions are a coroutine-aware function kind, a `yield` terminator,
an explicit coroutine value/resume operation, and frame-slot primitives for
values that must live in the coroutine frame. MIR does not need to mention LLVM
coroutine intrinsics directly.

---

## 1. Current Backend Snapshot

The existing backend is in `lang/backend_llvm/coroutines/` and is built around
LLVM coroutine intrinsics, not a custom switch-state-machine anymore.

Current physical representation:

- `Coroutine<T>` lowers physically as a raw `ptr` handle
  (`CORO_GENERIC_PTR`), not as a stored `{handler, env}` pair.
- Applying a coroutine value, `co ()`, is special-cased in
  `call_coroutine_value` and calls `codegen_handle_resume(handle, T)`.
- The resume "handler" exists as compiler-generated control flow at the call
  site, not as a function pointer stored in each coroutine value.

Current coroutine constructor ABI:

```c
ptr coro_name(i64 *frame_size_out, arg0, arg1, ...)
```

- The hidden first parameter receives `llvm.coro.size.i64()`.
- The function returns the raw coroutine handle from `llvm.coro.begin`.
- Coroutine constructor application usually goes through
  `coro_create_with_reset_closure`, which also records reset metadata in the
  promise.

Current promise layout:

```c
// CORO_PROMISE_TYPE(T)
{ T yield_value, i1 is_done, ptr reset_fn, ptr args_ptr }
```

- Field 0 stores the raw yielded `T`.
- Field 1 is an explicit stop/done flag used in addition to `llvm.coro.done`.
- Field 2 is a reset function pointer.
- Field 3 is reset argument storage.

The current `yield` path stores raw `T` at the promise address, then emits
`llvm.coro.save` and `llvm.coro.suspend`. Resume reads the promise's field 0 and
wraps it as `Some`; done paths return `None`.

Frame placement currently works by placing allocas before the coroutine frame is
finalized:

- constructor parameters are spilled to allocas before `llvm.coro.begin`;
- yield-boundary bindings use `__handle_yield_boundary_crossing_binding`, which
  creates an alloca with a temporary builder positioned before the coroutine
  anchor;
- array literals inside a coroutine use `__allocate_coroutine_array`, also
  inserting their backing storage before the anchor.

Current special cases:

- `yield inner_coroutine` drains the inner coroutine with an explicit
  `done/resume/promise/yield` loop.
- recursive `yield self(args)` mutates the parameter spill slots and branches
  back to the coroutine `start` block.
- `cor_loop`, `cor_map`, `cor_filter`, `iter` for lists/arrays, zip helpers,
  and reset/take/stop helpers are hand-written LLVM wrapper coroutines in
  `coroutine_extensions.c`.
- Resettable combinators use `reset_fn` and `args_ptr` stored in the promise;
  `cor_loop` recreates an inner coroutine and restores the original handle with
  `coro_emit_memcpy_restore`.

This shape is a useful reference, not a requirement. MIR should choose the form
that makes analysis and lowering clearest while preserving these observable
features: resumable `co ()`, nested yield-from, recursive coroutine loops,
resettable combinators, stop/done behavior, and values surviving yield
boundaries.

---

## 2. Current Source-Level Model

Type inference already marks coroutine lambdas:

- `AST_YIELD` sets `AST_LAMBDA.is_coroutine = true`.
- The lambda function type gets `FN_ATTR_COROUTINE_CONSTRUCTOR`.
- The yielded element type is wrapped as `Coroutine<T>`.
- `AST_LAMBDA.yield_boundary_crossers` records identifiers used after crossing
  yield boundaries.

The MIR pipeline should reuse that metadata at first, then replace it with a
MIR-level liveness analysis once coroutine CFG is explicit enough.

Current runtime-facing shape:

- Source value type: `Coroutine<T>`.
- Call surface: `co () : Option<T>`.
- LLVM old backend mostly treats coroutine instances as raw handles and uses
  helper lowering when the value is applied.

Candidate logical MIR ABI shape:

```c
Coroutine<T> ~= {
  handler: Option<T> (*)(ptr env),
  env: ptr
}
```

For LLVM coroutine-backed instances, `env` is the LLVM coroutine handle returned
by `llvm.coro.begin`, and `handler` is a yield-type-specialized resume wrapper.
This intentionally mirrors closures: a coroutine is a callable pair of code plus
environment, where the environment is the suspended frame/handle.

This is not a requirement for LLVM lowering. It is attractive because it lets
coroutine application reuse the same mental model as closure application. If a
raw-handle plus typed `coro.resume_value` operation is simpler, use that. The
hard requirement is that MIR preserves a typed coroutine value and makes resume
explicit enough for ownership and lowering.

---

## 3. MIR Data Model Additions

Add coroutine metadata to `MirFunction` rather than encoding everything in names:

```c
typedef enum {
  MIR_FUNCTION_NORMAL,
  MIR_FUNCTION_COROUTINE_CONSTRUCTOR,
  MIR_FUNCTION_COROUTINE_RESUME_HANDLER,
} MirFunctionKind;

typedef struct {
  Type *yield_type;          // T in Coroutine<T>
  Type *promise_type;        // logical yielded storage, not LLVM-specific
  MirValueId state_value;    // abstract coroutine state/frame if materialized
  MirFunctionId resume_fn;   // optional Option<T> (state/env)
  MirFrameSlotVec frame_slots;
} MirCoroutineInfo;
```

Suggested rule:

- A function whose source type has `FN_ATTR_COROUTINE_CONSTRUCTOR` becomes
  `MIR_FUNCTION_COROUTINE_CONSTRUCTOR`.
- Source-level users see and pass around the full logical `Coroutine<T>` value.
- LLVM lowering may choose either:
  - an internal LLVM coroutine body that returns a raw handle;
  - a small constructor wrapper that builds `{handler, env}`;
  - or a raw handle physical value plus a typed resume operation.
- MIR should not encode `llvm.coro.id`, `llvm.coro.begin`, `llvm.coro.save`, or
  related intrinsics. It only exposes the abstract storage/control operations
  the lowerer needs.

Keep generic specialization unchanged: coroutine constructors can still be
generic and specialize on demand like normal functions.

---

## 4. MIR Coroutine Frame Primitives

MIR should make coroutine frame state explicit without committing to LLVM's
coroutine ABI. The lowerer can later map these slots to LLVM coroutine-frame
allocas, a custom state struct, or another representation.

Add frame slot metadata:

```c
typedef enum {
  MIR_FRAME_SLOT_PARAM,
  MIR_FRAME_SLOT_CAPTURE,
  MIR_FRAME_SLOT_YIELD_CROSSER,
  MIR_FRAME_SLOT_TEMP,
} MirFrameSlotKind;

typedef struct {
  MirFrameSlotId id;
  const char *name;
  Type *type;
  MirFrameSlotKind kind;
  MirOperandUse init_use;
} MirFrameSlot;
```

Add MIR operations or instruction kinds equivalent to:

```text
frame.init_slot $slot, %value     // constructor-time initialization
frame.store $slot, %value         // update live coroutine state
frame.load $slot : T              // read state value
```

Semantics:

- `frame.init_slot` is used for constructor arguments and coroutine captures.
- `frame.store` is used for mutable or loop/recursive coroutine state updates,
  including recursive `yield self(args)` rewriting parameter slots.
- `frame.load` is what the coroutine body uses when reading a value that lives
  in the frame.
- Frame-slot operations participate in operand ownership like ordinary stores:
  consumed values move into the slot; borrowed managed values must be duplicated
  or retained if the slot owns them.

Constructor initialization:

1. When a coroutine constructor is called, evaluate constructor arguments and
   captured values at the call site.
2. Lower them as constructor inputs to the coroutine instance.
3. At coroutine construction, emit `frame.init_slot` for every parameter and
   capture slot.
4. Bind the coroutine body's parameter/capture names to `frame.load` from those
   slots, not to ordinary function params or closure env fields.

This is the MIR-level requirement. Whether those slots become LLVM allocas
before `llvm.coro.begin`, fields in a custom frame struct, or another mechanism
is a lowering decision.

---

## 5. New MIR Terminator: `yield`

Add a terminator:

```c
MIR_TERM_YIELD
```

Minimum payload:

```c
typedef struct MirTerminator {
  MirTermKind kind;
  MirValueId value;       // yielded value
  MirBlockId target;      // resume continuation
  ...
} MirTerminator;
```

Semantics:

- `yield %v, bbN` publishes `%v` as the next yielded value and when the
  coroutine is resumed continues at `bbN`.
- It is a terminator because control leaves the current activation until a later
  resume.
- For MIR CFG analyses, its normal successor is the resume continuation.
- MIR does not spell out the suspend-return or destroy/cleanup mechanics. Those
  are lowering details. MIR only records the yielded value and the resume
  continuation.

Operand ownership:

- The yielded operand is a borrow for scalar values.
- For managed values, use the same policy as return: yielding exposes the value
  to the caller, so yielded storage needs a valid owned or retained value until
  the next resume or final cleanup.
- Perceus lowering should treat `yield` as a boundary root. A consumed yielded
  value must be either stored in the promise with ownership transferred, or
  duped before storing if the source value remains live after the yield.

Printing and visitors:

- `mir_term_for_each_operand` must visit `yield.value`.
- Terminator successor utilities must include `yield.target`.
- MIR dump should print something like:

```text
yield %12, bb4 ; ops [%12:yield/borrow#0]
```

Add a new operand role such as `MIR_OPERAND_ROLE_YIELD` if this makes ownership
metadata clearer than overloading `RETURN`.

---

## 6. Lowering `AST_YIELD` to MIR

Initial lowering:

1. Lower the yielded expression to a MIR value.
2. If the yielded value has type `Coroutine<T>`, lower as "yield from" instead
   of yielding the coroutine object itself.
3. Create a continuation block.
4. Terminate the current block with `MIR_TERM_YIELD(value, continuation)`.
5. Continue MIR construction in the continuation block.

For source bodies that currently use `yield` as a statement, no MIR result is
needed. If the parser/typechecker permits `yield` in expression position, return
a dummy `Void` value in the continuation for now and reject non-statement
contexts later if needed.

Nested coroutine/yield-from lowering:

- Phase 1: desugar nested `yield coro_expr` into an explicit MIR loop:
  - check whether inner coroutine is done;
  - resume it;
  - read its yielded promise;
  - `yield` that value from the outer coroutine;
  - loop until inner returns `None`.
- Phase 2: introduce a dedicated `MIR_OP_CORO_RESUME` and possibly
  `MIR_TERM_YIELD_FROM` only if the explicit loop becomes too noisy.

The explicit-loop form is preferable first because it exercises existing block,
phi, call, and ownership machinery.

Recursive coroutine lowering:

- Preserve the current backend behavior for `yield self(args)`:
  - evaluate new arguments;
  - store them into the constructor parameter frame slots;
  - branch back to the coroutine `start` block;
  - do not allocate a fresh nested coroutine.
- In MIR this is cleaner as a branch to the start block after frame-slot stores,
  not as a normal `yield` terminator.

---

## 7. Coroutine Frame Placement

This is the critical ownership/lifetime rule:

Any value that appears in coroutine constructor arguments, any value closed over
by a coroutine lambda, or any value whose definition reaches a use across a
`yield`, must live in the coroutine frame.

This is not the same as ordinary stack allocation and not the same as heap
allocation. Add an explicit allocation/storage category:

```c
typedef enum {
  MIR_STORAGE_SSA,
  MIR_STORAGE_STACK,
  MIR_STORAGE_HEAP,
  MIR_STORAGE_CORO_FRAME,
} MirStorageClass;
```

For allocation sites, this should eventually become:

```c
EA_STACK_ALLOC
EA_HEAP_ALLOC
EA_CORO_FRAME_ALLOC
```

The old AST escape analysis currently treats crossing a yield boundary as heap.
For MIR coroutines, that should become coroutine-frame placement where possible.
The visible MIR artifact should be frame-slot operations, not heap-vs-stack
guessing.

Frame slot sources:

1. Constructor parameters.
   - Every non-void parameter of a coroutine constructor gets a frame slot.
   - Constructor lowering emits `frame.init_slot` for each argument.
   - The body binds the source parameter pattern to `frame.load` from that slot.
   - Ownership of consumed constructor arguments moves into the frame.

2. Coroutine closed-over values.
   - If a coroutine lambda closes over values, those values should be
     initialized into coroutine frame slots when the coroutine instance is
     constructed, using the same mechanism as constructor arguments.
   - Avoid representing coroutine captures as an ordinary closure env whose
     lifetime is separate from the coroutine frame.
   - This applies regardless of whether the final physical coroutine value is a
     raw handle or a `{handler, handle}` pair.

3. Yield-boundary crossers from AST metadata.
   - Use `AST_LAMBDA.yield_boundary_crossers` for the first implementation.
   - Map those identifiers to MIR values/allocations and force frame slots.
   - Rewrite post-yield uses to `frame.load`.

4. MIR liveness across yields.
   - Later, compute this directly from MIR:
     - assign each block/yield a suspension index;
     - find defs that dominate a yield and have uses reachable after that yield;
     - mark those values as `MIR_STORAGE_CORO_FRAME`;
     - include values referenced by phis after a yield.

Managed aggregate warning:

Spilling a fat array/list/string struct into the coroutine frame is not enough
if its backing pointer still points into the caller's stack. For values passed
as coroutine constructor args:

- Phase 1 conservative rule: if a managed aggregate argument was constructed in
  the caller and cannot be directly placed in the coroutine frame, heap-promote
  its backing storage rather than allowing caller-stack storage to escape.
- Phase 2 direct placement: when the constructor argument expression is an
  allocation expression that can be recognized at the call site, construct its
  backing storage directly as a coroutine-frame allocation in the coroutine
  constructor.

For arrays, the whole fat value and its data buffer must have compatible
lifetimes. For lists, every cons node reachable from the constructor argument
must either be owned by the coroutine frame or be heap-owned with correct RC.

---

## 8. MIR-Level Coroutine Value

One possible representation mirrors closure values:

```text
construct.coroutine { handler, handle } : Coroutine<T>
extract.coroutine_handler %co
extract.coroutine_env %co
```

That can reuse the existing closure physical type if it proves useful:

```llvm
%Coroutine = type { ptr, ptr }
```

or use the existing `%Closure = { ptr, ptr }` ABI type if that lets coroutine
application share closure-call machinery.

The other viable representation is the current backend's raw handle:

```text
Coroutine<T> physical value: ptr handle
```

with `co ()` lowered through a typed resume operation:

```text
%res = coro.resume_value %handle : Option<T>
```

This may be the simplest LLVM lowering. The `{handler, handle}` form may still
be simpler for MIR analysis if it reuses closure code. Keep the decision local
to coroutine value lowering; frame-slot semantics are independent either way.

If using `{handler, handle}`, then calling `co ()` lowers to:

```text
%handler = extract.coroutine_handler %co
%env = extract.coroutine_env %co
%res = call %handler(%env) : Option<T>
```

Handler signature:

```c
Option<T> resume_handler(ptr handle)
```

The handler:

1. checks `llvm.coro.done(handle)`;
2. if done, returns `None`;
3. calls `llvm.coro.resume(handle)`;
4. checks done again;
5. reads the promise via `llvm.coro.promise`;
6. wraps the raw yielded value in `Some`.

This matches `codegen_handle_resume` in the old backend.

---

## 9. LLVM Lowering for Coroutine Constructors

Lowering a `MIR_FUNCTION_COROUTINE_CONSTRUCTOR` should preserve the old
backend's intrinsic lifecycle, but the public MIR value does not have to match
the old raw-handle ABI:

1. Declare/create the LLVM coroutine function.
   - Internal coroutine body can use the current backend signature:
     `ptr (i64 *frame_size_out, frame-slot inputs...)`.
   - Public constructor lowering can return either a raw handle or a wrapper
     value, depending on the chosen coroutine value representation.
   - Mark with the current coroutine function attribute
     (`PRESPLIT_COROUTINE_KIND_ID` in the old backend).

2. Create standard blocks.
   - `entry`
   - `cleanup`
   - `suspend`
   - `initial.return`
   - `start`

3. In `entry`, allocate promise storage.
   - Keep the current `CORO_PROMISE_TYPE(T)` layout:
     `{T yield_value, i1 is_done, ptr reset_fn, ptr args_ptr}`.
   - Initialize `is_done` to false.
   - Initialize `reset_fn` and `args_ptr` to null unless the constructor
     application fills them later.

4. Emit `llvm.coro.id`.
   - Pass the promise pointer.
   - Pass null `coroaddr`/`fnaddr` unless reset support requires otherwise.

5. Lower MIR frame-slot primitives.
   - `frame.init_slot` for constructor parameters and coroutine captures must
     become storage that lives in the coroutine frame.
   - `frame.store` updates that frame-resident storage.
   - `frame.load` reads from it.
   - If using LLVM coroutine intrinsics, materialize the corresponding storage
     before `llvm.coro.begin` so LLVM can promote it into the coroutine frame.

6. Emit `llvm.coro.size`, allocate the frame memory, and call
   `llvm.coro.begin`.
   - Store the frame size through the hidden `frame_size_out` parameter before
     returning the handle.

7. Emit initial suspend:
   - `llvm.coro.save(handle)`
   - `llvm.coro.suspend(token, false)`
   - switch:
     - `0` -> `start`
     - `1` -> `cleanup`
     - default -> `initial.return`, then `suspend`

8. Lower the MIR body from `start`.

9. At normal function completion, emit final suspend:
   - natural completion is detected by `llvm.coro.done`; the explicit
     `is_done` promise flag is currently used by `cor_stop`;
   - `llvm.coro.save(handle)`
   - `llvm.coro.suspend(token, true)`
   - switch to final return or cleanup.

10. Emit cleanup and suspend blocks:
    - cleanup calls `llvm.coro.free(id, handle)` then `free`;
    - suspend calls `llvm.coro.end(handle, false, token none)`;
    - return the raw handle from the internal coroutine function.

Then materialize the public `Coroutine<T>` value. If the physical value is a raw
handle, return that handle. If the physical value is `{handler, handle}`, wrap
the handle with the typed resume handler.

Important LLVM placement rule:

Frame slots should lower as allocas that LLVM's coroutine passes can promote
into the coroutine frame. Do not model these as normal caller-stack allocas.
The lowerer should emit them in the coroutine function entry at the same stage
as the old backend's parameter spill allocas.

---

## 10. LLVM Lowering for `MIR_TERM_YIELD`

Lower `yield %v, bb.resume` as:

1. Cast/store `%v` into promise field 0. The current backend often stores raw
   `T` through the promise pointer directly because field 0 starts at offset 0;
   MIR lowering should prefer an explicit field-0 GEP for clarity.
2. Emit:

```llvm
%save = call token @llvm.coro.save(ptr %handle)
%state = call i8 @llvm.coro.suspend(token %save, i1 false)
switch i8 %state, label %yield.return [
  i8 0, label %bb.resume
  i8 1, label %cleanup
]
```

3. `yield.return` branches to the function-level `suspend` block.
4. The MIR resume block lowers normally.

If yielding a managed value:

- Store the owned/retained representation into the promise.
- If the value remains live in `bb.resume`, insert/require a dup before storing.
- On final cleanup, drop any currently stored promise value if ownership rules
  require it.

---

## 11. Coroutine Builtins and Combinators

The old backend has special handlers for:

- `iter` / list and array iteration;
- `cor_loop`;
- `cor_map`;
- `cor_filter`;
- `cor_zip` / `cor_zip_struct`;
- reset/stop/take-style helpers.

MIR should not lower these through ordinary extern calls. Treat them like the
other compiler-special builtins:

- each builtin gets a MIR handler that emits either:
  - a generated coroutine constructor MIR function, or
  - a call to a runtime/helper function that returns `Coroutine<T>`.
- generic builtin declarations continue to exist only to provide source types.

Implementation order:

1. Direct user coroutine lambdas.
2. Calling/resuming a coroutine value.
3. Nested `yield` / yield-from.
4. `iter` for arrays and lists.
5. `cor_loop`.
6. `cor_map`.
7. `cor_filter`, zip, and reset/stop helpers.

Reset compatibility:

- Preserve `reset_fn`/`args_ptr` fields in the promise until there is a better
  MIR-level reset representation.
- For resettable combinators, generated MIR coroutine constructors need a hidden
  frame-size out-param just like the current LLVM helpers.
- `cor_loop` currently depends on recreating an inner coroutine and memcpying
  the new frame over the original handle, so do not drop frame-size plumbing
  from the initial MIR design.

---

## 12. Perceus and Ownership Integration

Coroutines add a third long-lived root in addition to returns and top-levels:
the coroutine frame.

Rules:

- Constructor argument ownership moves into the frame if consumed.
- A borrowed constructor argument that is stored in the frame must be duplicated
  or otherwise retained.
- Values live across yield boundaries are owned by frame slots.
- A yielded managed value is owned by the promise until the caller consumes the
  returned `Some`, or until overwritten/finalized.
- Final suspend and cleanup must drop frame-owned managed values exactly once.

For the first MIR implementation, prefer conservative retention:

- dup when storing borrowed managed values into the promise/frame;
- drop frame slots during coroutine cleanup;
- avoid frame slot reuse until ownership summaries are robust.

Later, frame slots can participate in reuse analysis the same way stack slots
and closure env slots do.

---

## 13. Pass Pipeline Requirements

The MIR LLVM lowerer must ensure coroutine intrinsics are run through the LLVM
coroutine passes.

The old `module_passes` currently has comments for:

```text
coro-early,coro-elide,coro-split,coro-cleanup
```

but normally chooses `default<O0>` or `default<O3>`. Before enabling MIR
coroutines broadly:

1. Verify whether the active LLVM version's `default<O3>` runs the required
   coroutine passes for this IR shape.
2. If not, prepend or explicitly run the coroutine pass sequence.
3. Add a debug/test mode that dumps pre- and post-pass IR for a simple
   coroutine, confirming there are no unlowered `llvm.coro.*` intrinsics in the
   executable module except legal runtime intrinsics if any remain.

---

## 14. Implementation Stages

### Stage A: MIR shape only

- Add `MIR_TERM_YIELD`.
- Add coroutine frame slot metadata.
- Add `frame.init_slot`, `frame.store`, and `frame.load` MIR primitives.
- Add dump, operand visitor, operand rewrite, terminator clone, successor
  handling, and validation support.
- Lower `AST_YIELD` to MIR inside coroutine functions.
- Mark coroutine functions in MIR metadata.
- Add MIR dump tests only, no LLVM lowering yet.

### Stage B: Constructor and Frame Initialization

- Lower coroutine constructor arguments into `frame.init_slot`.
- Lower coroutine closed-over values into `frame.init_slot`.
- Bind coroutine body params/captures through `frame.load`.
- Rewrite recursive coroutine state updates as `frame.store` plus branch to
  `start`.
- Add MIR tests before LLVM lowering.

### Stage C: Coroutine value ABI

- Choose the concrete MIR coroutine value representation:
  - raw handle plus typed `coro.resume_value`; or
  - `{handler, handle}` if closure-call reuse makes this simpler.
- Lower `co ()` through the chosen representation to behavior equivalent to
  `codegen_handle_resume`.
- MIR/LLVM tests for a manually constructed coroutine value can land here.

### Stage D: Basic LLVM Coroutine Lowering

- Lower coroutine constructor function setup:
  - hidden `i64 *frame_size_out` parameter;
  - promise;
  - `coro.id`;
  - `coro.size`;
  - frame allocation;
  - `coro.begin`;
  - initial suspend;
  - final suspend;
  - cleanup/suspend blocks.
- Lower `MIR_TERM_YIELD`.
- Support simple no-arg coroutines:

```ylc
let c = fn () -> yield 1; yield 2;;
let co = c ();
co () == Some 1 && co () == Some 2 && co () == None
```

### Stage E: Frame Slot Lowering Completeness

- Lower `frame.init_slot`, `frame.store`, and `frame.load` through the chosen
  LLVM representation.
- Use `yield_boundary_crossers` to force frame slots.
- Add special coroutine-frame backing allocation for arrays, matching
  `__allocate_coroutine_array`.
- Add conservative managed aggregate handling for constructor args.
- Add tests where an arg is yielded after one or more suspension points.

### Stage F: Nested yield/yield-from

- Lower `yield inner_coroutine` as explicit resume/yield loop.
- Cover recursive coroutines like Fibonacci/geometric tests.
- Lower recursive `yield self(args)` as frame-slot update plus branch
  to `start`.

### Stage G: Builtin coroutine combinators

- Implement MIR handlers for `iter`, `cor_loop`, `cor_map`, then the rest.
- Prefer generated MIR coroutine functions over hand-written LLVM helper IR
  where practical.
- Preserve reset metadata and frame-size out-param behavior while porting.

### Stage H: Ownership tightening

- Integrate frame slots into MIR escape and Perceus analysis.
- Drop frame-owned values in cleanup.
- Track promise ownership and overwrites.
- Add stress tests for arrays/lists/closures crossing yields.

---

## 15. Test Plan

Start with `test/test_scripts/10_coroutines.ylc`, but stage it so failures are
diagnostic:

1. MIR dump tests:
   - coroutine lambda is marked as coroutine constructor;
   - `yield` appears as terminator;
   - continuation blocks are explicit;
   - constructor args initialize frame slots;
   - coroutine closed-over values are marked frame slots, not ordinary closure
     env fields.
   - post-yield uses of frame values are `frame.load`.

2. LLVM shape tests:
   - constructor function contains `llvm.coro.id`;
   - contains `llvm.coro.begin`;
   - each yield contains `llvm.coro.save` and `llvm.coro.suspend`;
   - resume handler calls `llvm.coro.done`, `llvm.coro.resume`, and reads
     `llvm.coro.promise`.

3. Runtime tests:
   - no-arg sequence coroutine;
   - parameter sequence coroutine;
   - value defined before yield and used after yield;
   - constructor arg array/list survives after caller frame returns;
   - nested yield-from;
   - recursive coroutine;
   - `iter` array/list;
   - `cor_loop`;
   - `cor_map`;
   - zip/struct zip.

4. Ownership tests:
   - yielded list/array is not dropped before caller observes it;
   - frame-owned list/array is dropped once on completion/destroy;
   - coroutine constructor consumes managed args exactly once;
   - repeated `None` calls after completion do not double-drop.

---

## 16. Open Questions

1. Should the user-visible MIR coroutine value be represented as a raw handle
   plus typed resume op, or as `{handler, handle}`?

   Preferred decision criterion: choose whichever makes MIR ownership analysis
   and LLVM lowering simpler. `{handler, handle}` may reuse closure machinery;
   raw handle may keep LLVM coroutine lowering smaller.

2. Should the promise store raw `T` or `Option<T>`?

   Preferred: keep current backend behavior. Promise field 0 stores raw `T`;
   resume handler wraps in `Some`; done paths return `None`. Keep the full
   four-field promise while reset/stop support depends on it.

3. Should `yield` be a terminator or an instruction with exceptional control
   flow?

   Preferred: terminator. It makes suspension boundaries explicit and prevents
   later instructions from being accidentally considered same-activation code.

4. How aggressive should frame placement be for managed aggregate constructor
   args?

   Preferred first implementation: conservative heap promotion for backing
   storage when direct frame placement is uncertain. Then add direct
   coroutine-frame construction for recognized literals/fills/cons chains.

5. Should coroutine frame slots be visible in MIR dumps?

   Preferred: yes. Frame placement is a major correctness property, so tests
   should assert it directly.
