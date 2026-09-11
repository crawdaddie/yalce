# Allocation & Lifetime Strategy

Discussion of how list/array literals and other heap-allocated objects should be
tracked, where allocation decisions should be made (frontend vs IR pass), and
the representation choices that unlock optimization. Conclusions and tradeoffs
are recorded here; nothing here is yet implemented.

Scope: `lang/backend_llvm/` allocation paths (`array.c`, `list.c`, `strings.c`),
the existing escape analysis (`lang/escape_analysis.c`), and the IR pass
pipeline (`lang/backend_llvm/jit.c`).

---

## 1. Current state

### Representation

Arrays are fat structs, not native LLVM arrays:

```c
// array.c:13
LLVMTypeRef codegen_array_type(LLVMTypeRef element_type) {
  return LLVMStructType((LLVMTypeRef[]){
      LLVMInt32Type(),                 // size
      LLVMPointerType(element_type, 0)  // data pointer
  }, 2, 0);
}
```

So an array value is `{ i32 size, T* data }`, passed by value as two registers.
The `data` buffer is a *separate* `malloc`/`alloca` (`array.c:140`, `array.c:137`),
and the struct just carries a pointer to it. Size is read via `extractvalue`
field 0 (`codegen_get_array_size`, `array.c:192`).

Lists and strings use analogous fat structs (`{ptr next-or-data, ...}`).

### Allocation decision

`find_allocation_strategy` (`common.c:36`) is the single gate:

```c
EscapeStatus find_allocation_strategy(Ast *expr, JITLangCtx *ctx) {
  if (expr->ea_md && ctx->stack_ptr != 0) {
    return ((EscapeMeta *)expr->ea_md)->status;   // EA_STACK_ALLOC or EA_HEAP_ALLOC
  }
  return EA_HEAP_ALLOC;   // default: heap when not analyzed
}
```

Key behavior: **unanalyzed → heap.** Anything without an `ea_md` annotation, or
any allocation at `stack_ptr != 0` without analysis, falls through to heap.

### Existing escape analysis

`lang/escape_analysis.c` is **intra-procedural**. It tracks an allocation from its
`AST_ARRAY`/`AST_LIST`/`AST_STRING` site through the *same* function body and marks
heap on four local signals:

| signal | where |
|---|---|
| `is_returned` | `escape_analysis.c:237-247` |
| `is_captured` by a returned closure | `escape_analysis.c:94-114` |
| crosses a yield boundary (coroutine) | `escape_analysis.c:132-141`, `218-226` |
| `is_mutable` via `array_set` | `escape_analysis.c:282-292` |

When an allocation is **passed as an argument to a call** (`AST_APPLICATION`),
the default arm does `break` — it records *nothing*. So any list/array passed to
any function call is forced to heap by the `unanalyzed → heap` fallback above.
This is the core gap: the analysis cannot reason about whether an object passed
to `Lists.fold` survives the call.

### What the IR pass pipeline already does

`module_passes` (`jit.c:252`) runs `LLVMRunPasses` with `default<O3>` (or `O0` in
debug). Everything compiles into **one module** (`ylc.top-level`, `jit.c:290`):
imported modules compile into the same `llvm_module_ref` (`module.c:203`), and
generic functions specialize on demand into it (`get_specific_callable`,
`function.c:522`). So by the time the pass pipeline runs, `fold`, `map`, and the
caller are concrete sibling functions in one module — whole-program analysis is
in scope.

### What `O3` already gives us (observed in `std/Arrays.ylc` IR)

From pre/post-optimization IR of the test module:

- **Tail-recursion → loop** on `fold` (the recursive `tail call @fold` became a
  back-edge with phi nodes; O(n)-stack recursion → O(1)-stack loop). Enabled by
  `set_tail_call_expressions` + `LLVMSetTailCall` (`function.c:131`, `:346`).
- **Inlining + IPSCP** into `@top`: `fold`/`last` over constant arrays
  const-folded to `true` at compile time (4 of 6 tests). Generic specialize +
  inline working.
- **Auto-vectorization** of `@map`'s fill loop (`<4 x i32>` stores,
  `!llvm.loop.isvectorized = 1`), enabled by `mem2reg` promoting the loop
  allocas to SSA IVs.
- **Function-attribute inference**: `readonly captures(none)` on `fold`'s
  fn-pointer arg, `memory(none)` on `anonymous_func`, `memory(read, ...)` on
  `last`. Interprocedural noescape inference happening automatically from
  visible bodies.
- **Recursion → single indexed load** in `last` (`arr[size-1]`).
- **Vector constant stores** for literal arrays (`<2 x double>`).

### What `O3` does NOT give us (observed gaps)

- **Dead `malloc`s survive.** Const-folded arrays' allocations linger because
  DSE can't remove `malloc` (no paired `free`, modeled as side-effecting). E.g.
  `%array_data_alloc = tail call ptr @malloc(i32 24)` for a `[1.,2.,3.]` whose
  fold already folded to `true`.
- **`strncmp` stays runtime** for two constant-buffer comparisons (libcall LLVM
  won't const-fold). Would fold to `true` if array equality went element-wise.
- **Dead function definitions linger** (`@fold`, `@map`, etc. all still
  `define`d but uncalled from `@top`) — because everything is
  `LLVMExternalLinkage`, blocking `GlobalDCE`.
- **No `free`** — arrays leak (expected; no GC path yet).

---

## 2. The three allocation roots

A per-frame decision: an allocation must be heap iff it is reachable from
something that **outlives the frame**. There are exactly three such roots:

| root | why it outlives the frame | current coverage |
|---|---|---|
| **top-level vars** | program lifetime | implicit (heap-by-default) |
| **coroutine state** | outlives the allocating frame across suspends | yes (`alloc_crosses_yield_boundary`) |
| **this frame's return value** | lives in the caller, which outlives this frame | partial (`is_returned`) |

The third is the common gap for array-returning functions (`map`, `filter`,
`rev`): the result array is reachable from the function's return, so it
escapes the function's frame even if it's not globally top-level-visible.

### "Visible to a top-level var" = transitive field-reachability

Reachability is over **values**, not over local `let` variables. Starting from
a top-level binding, follow its value's fields transitively (tuple/array
elements, `ptr` fields, closure env slots). A top var holding a `Bool` cannot
reach an array — `Bool` has no array-typed field.

Worked example (`std/Arrays.ylc` `test` module): every top-level binding is a
`Bool` (the result of `==`). From a `Bool` you cannot reach any array.
Therefore **no array in that module is reachable from a top-level var**, and
under the root rule, nothing there strictly requires the heap. The only
constraint forcing heap *anywhere* is `map`/`filter`'s result escaping their
own frame (root 3) — which the out-param form (§5) eliminates.

---

## 3. Representation decision: keep `{i32, T*}`, use `[N x T]` for backing storage

### Why not native `[N x T]` as the array type

LLVM `[N x T]` is fixed-length, value-typed, contiguous. Three properties of the
language's arrays break it:

1. **Runtime, variable length** in the general case. `array_fill n f`
  (`array.c:296`), `array_range`, `array_offset` take `n` as a runtime argument.
   `[N x T]` requires `N` constant at type-construction. Real programs use
   runtime sizes: `array_fill_const (lines * 6) 0.0` (`StarMap.ylc:89`),
   `array_fill (in_feat * out_feat) ...` (`linear_regression.ylc:26`).
2. **Separate heap/stack storage of `data`.** The escape-analysis story hinges
   on `data_ptr` being independently `malloc`'d or `alloca`'d. With `[N x T]`
   the storage *is* the array — can't choose stack-vs-heap for buffer vs handle,
   can't resize, can't slice (`array_offset` shares the buffer with a new size,
   `array.c:442-450`).
3. **Passing semantics.** `[N x T]` is passed by value as N registers (or
   `byval` to a hidden pointer). `{i32, T*}` is two registers with shared
   buffer-by-reference — which is what makes `array_set` mutation and shared
   closures work.

So native arrays are out for the *general* array type. The fat struct is the
correct representation — same shape as Rust `&[T]` and Go slices.

### Where `[N x T]` *does* help: as the backing `alloca` for const-size arrays

The win is `sroa` (scalar replacement of aggregates). The mechanism:

- **`alloca [N x T]`** (size in the *type*): `sroa` slices it into N independent
  slots. `arr[i]` with constant `i` promotes to a direct SSA load of slot `i` —
  the allocation disappears, replaced by N SSA scalars. Robust across pass
  orderings; works at `O0`/`O2`.
- **`alloca T, const-N`** (size as *operand*, `LLVMBuildArrayAlloca`): semantically
  the same size, but `sroa` sees the array dimension as an operand. Needs an
  earlier pass to fold the constant and canonicalize to `[N x T]` before `sroa`.
  Pass-ordering dependent; fails at `O0`.
- **`malloc T, n`**: `sroa` never touches heap.

### Why the fat struct "hides" arrays from optimization

It's not the struct's *fields* — `extractvalue`/`insertvalue` on `{i32, ptr}`
are SSA ops, cheap. It's the **pointer indirection the struct forces**: the
buffer is a *separately-allocated, pointer-referenced, variably-sized* object,
and `sroa` does not follow pointers. Specifically:

- The struct value is built with `insertvalue`, never an `alloca` — `sroa` never
  sees it.
- The buffer is a distinct `alloca T, %n` / `malloc`. `sroa` processes it
  independently. Whether it can slice it depends on its *type*.
- `alloca T, i32 %n` is variable-sized → `sroa` can't partition → element
  accesses stay as memory ops against the buffer.
- Every `arr[i]` re-extracts `%ptr` from the struct at the call site, eroding
  the "this is one object" property that alias analysis relies on.

Emitting `alloca [N x T]` for the *buffer* (size in the type) removes the
variable-size property and lets `sroa` dissolve it. The surface struct and
`codegen_get_array_size` stay unchanged.

### Concrete change

At the allocation site (`array.c:136` for literal arrays, `array.c:216` for
`array_fill`), gate the `[N x T]` form on a constant size and stack promotion:

```c
if (find_allocation_strategy(ast, ctx) == EA_STACK_ALLOC && ctx->coro_ctx == NULL) {
  if (LLVMIsConstant(size_const)) {
    unsigned n = (unsigned)LLVMConstIntGetZExtValue(size_const);
    LLVMTypeRef arr_ty = LLVMArrayType(element_type, n);
    LLVMValueRef slot = LLVMBuildAlloca(builder, arr_ty, "array_fixed");
    data_ptr = LLVMBuildBitCast(builder, slot,
                                LLVMPointerType(element_type, 0), "array_data");
  } else {
    data_ptr = LLVMBuildArrayAlloca(builder, element_type, size_const,
                                   "array_dyn");
  }
} else {
  data_ptr = LLVMBuildArrayMalloc(builder, element_type, size_const,
                                  "array_data_heap");
}
```

Surface type and size getter unchanged. The `i32` is still `ConstantInt` for
literals, so `codegen_get_array_size` folds to a constant. What changes is only
what `data_ptr` aliases.

### On `llvm.objectsize` for size

No LLVM facility returns the *element count* of a `{i32, T*}` fat array.
`llvm.objectsize` returns *bytes* for `alloca`/`malloc`/globals and chases the
pointer, not the fat struct — wrong number (bytes vs count) and wrong object.
Keep `codegen_get_array_size` (`extractvalue` field 0). It's one instruction,
folds to a constant when static, honest when runtime. That is the right
runtime-size mechanism for this representation.

---

## 4. Lifetime tracking: four layers of increasing generality

The current four boolean flags (`is_returned`, `is_captured`, `is_mutable`,
`crosses_yield_boundary`) are a degenerate, syntactic, intra-procedural
points-to analysis. Each layer below is strictly more general.

### Layer 1 — `FN_ATTR_NOESCAPE` on parameters (cheap, manual, accurate)

Add a per-argument escape flag to the function type:

```c
// type.h — extend T_FN
struct {
  struct Type *from;
  struct Type *to;
  FnAttributes attributes;
  uint64_t noescape_args;   // bitmask: bit i set => arg i does not escape
} T_FN;
```

A function asserts "argument *i* is not retained after I return." Annotation:
`let fold = fn f res a:noescape -> ...`. At a call site, if the argument's bit
is set, passing that allocation to the call does **not** mark it escaping.

One-rule change in `ea()` at the `AST_APPLICATION` arm (`escape_analysis.c:277`):
instead of `break`, look up the callee type's `noescape_args` and suppress the
escape for the bit-set args.

- **Pro:** correct-by-construction, immediate `test11` win, minimal code.
- **Con:** manual. Requires annotating externs and library functions by hand.

### Layer 2 — Infer `noescape` for source-level functions (the real win for `std`)

Since `Lists.fold`, `map`, `filter` are source you compile (not extern), infer
the bit automatically. Generalize `ea()` to treat **function parameters as the
allocations being tracked**: seed `lambda_ctx.allocations` with one `Allocation`
per parameter, analyze the body, and write back `noescape_args` from each
param's flags. A parameter is noescape iff it's not returned, not captured by
an escaping closure, not stored into a longer-lived object.

Order: analyze callees before callers (module-dependency order; `std` is
compiled first). For recursion, a monotone fixed point — a param only "decays"
from noescape→escape, never back, so it converges (one pass for `std/Lists`).

- **Pro:** automatic; solves `test11` without annotation; gives the
  inter-procedural summaries for all of `std`.
- **Con:** needs callee-before-caller ordering; needs the param-seeding
  generalization of `ea()`; recursion needs a fixed point.

### Layer 3 — "Returns the same object" (identity/throughput)

Detect that a parameter *flows directly to the return* and tag it
`is_returned_identity` rather than blanket `escapes`. Matters for two patterns:
`fn l -> l` (param returned by identity) and `fn l -> (l, compute(l))` (param
returned inside a tuple). The caller must heap-allocate, but the callee does no
allocation of its own.

At a call site `let r = identity_fn(literal)`, if the param is
`is_returned_identity`, the literal's escape depends on what happens to `r`, not
on the call. Propagate: the allocation originally at the literal is now "owned"
by `r` and gets re-analyzed in the caller's scope. This is **allocation
forwarding** — turns "must heap because passed to a function" into "stack if
`r` itself doesn't escape."

- **Pro:** composes with Layer 2; handles the specific case the user named.
- **Con:** needs an `is_returned_identity` flag and call-site allocation
  forwarding; more bookkeeping than Layers 1-2.

### Layer 4 — Region-based lifetimes (Tofte–Talpin / Cyclone)

Stop tracking individual objects; track **regions** tied to scopes. Every
allocation assigned to a region; a region freed wholesale when its owner scope
exits. An allocation escapes iff a pointer to region R is reachable from region
R' whose lifetime outlives R. Handles nested data and gives deterministic
teardown (bulk-free the region) — valuable in a JIT where you'd otherwise
sprinkle individual `free`s.

- **Pro:** most general for arbitrary aliasing/nesting; deterministic teardown;
  handles the cases per-object analysis can't.
- **Con:** needs an effect system to infer region assignments; non-trivial.
  Most "general" in the sense of handling arbitrary aliasing, but heaviest lift.

### Layer 5 — Ownership / affine lifetimes (the Rust direction)

Most compositional and decidable: each object has one owner; lifetime = owner's
scope; borrowing is a temporary non-owning reference bounded by the owner's
scope. "Escaping" becomes "ownership transferred to a longer-lived scope."
Makes lifetime tracking *intraprocedural by construction* — no whole-program
analysis — because the type system carries the information.

- **Pro:** scales to arbitrary aliasing without context-sensitivity blowup;
  subsumes Layers 1-4 as runtime checks of what the type system guarantees
  statically.
- **Con:** a *type-system change*, not a backend pass. Heaviest lift of all;
  different project.

### Beyond — points-to abstraction underneath all the above

All layers need "can variable X point to allocation A?" Two choices:
**allocation-site abstraction** (current model — one abstract location per
`AST_ARRAY`/`AST_LIST` literal, fast, imprecise under loops) vs
**flow-sensitive points-to** (per-program-point sets, precise, expensive).
Stay allocation-site-based but add **field sensitivity** — so `l[0] := ...`
correctly attributes liveness to the container, not a phantom field. The
`array_set` heuristic (`escape_analysis.c:282`) is a hand-rolled, unsound
version of field sensitivity; making it a real points-to field edge fixes the
unsoundness and handles tuple/list mutation through aliases.

### Priority

| layer | what it solves | effort |
|---|---|---|
| 1 | `test11` via manual annotation | low |
| 2 | `test11` + all of `std` automatically | medium |
| 3 | pass-through/identity-return functions | medium |
| 4 | nested data + deterministic JIT teardown | high |
| 5 | compositional, no whole-program analysis | very high (language change) |

Do 1 + 2 first (they directly solve the motivating example); 3 as a follow-on;
4/5 only if the per-object model proves insufficient.

---

## 5. Where the analysis should live: frontend vs IR pass

### What an IR pass CAN do

- **Stack promotion via `mem2reg`/`sroa`/`promote-alloca`.** If literals are
  lowered to `alloca` and the escape is provable in IR, `sroa` promotes
  single-use allocas to SSA. Only helps if lowered to `alloca` first.
- **Existing LLVM escape analysis.** `bdce`/`dse`/`globals-aa` reason about
  pointer reachability. For `test11`, after `fold` is inlined (which `O3` does
  for source-level `fold`), the inlined body matches the list, returns a `Bool`,
  and never stores the list pointer — then DSE+mem2reg can drop the alloca.
  Requires zero custom analysis if the literal is an `alloca` and `fold`
  inlines.
- **One-line baseline:** lower literals to `alloca` by default, gate only on
  existing `EA_STACK_ALLOC`, let `O3` clean up. Worth measuring first.

### What an IR pass CANNOT do

- **Inlined-away callers lose the "does it escape" question.** After `O3`
  inlines `fold` into `test11`, "does `fold` escape its argument?" is no longer
  a question the IR poses. Good for `test11`, but means you can't *summarize*
  `fold` and reuse the summary. For the `noescape`-bit-on-the-type story, you
  need the summary *before* inlining — at the AST/type level.
- **Closure capture is invisible in lowered IR.** `new_clos`'s correctness
  hinges on "the inner closure captures `y`." After lowering that's a
  `{ptr fn, ptr env}` struct + `StructGEP`/store into an env record. An IR
  escape analysis must rediscover "this `env` pointer flows to a returned
  value" by chasing GEPs and stores — rebuilding the AST-level fact you
  already have cheaply (`is_captured`).
- **No type/ownership info in IR.** `T_FN` can carry `noescape_args`/region/
  ownership annotations; LLVM types are erased to `ptr`/struct shapes. The
  borrow/region/ownership design is a *type-system* fact; by IR it's been
  lowered away. This is why escape analysis in mature compilers (Rust, Swift,
  OCaml) is a **frontend** pass that emits annotations the backend honors.

### Division of labor

| concern | where | why |
|---|---|---|
| `noescape_args` / region / ownership — the type-system fact | `type.h` (`T_FN`), `escape_analysis.c` | info about function *types*, known before codegen, consumed by the allocator decision |
| inference of the bit for source functions | `escape_analysis.c`, generalized to seed params | needs the match/let/capture structure that exists only on the AST |
| the allocation decision (`alloca` vs `malloc`) | `array.c:136`, `list.c`, `strings.c:798` | gated on `find_allocation_strategy`; keep, feed it the annotation |
| cleanup / verification (kill dead allocas, free regions) | an LLVM pass via the existing C++ scaffolding | what IR is *good* at: once the frontend said "this may be stack," the IR pass confirms it (nothing escaped post-inline) and removes it |

**Frontend decides intent (could be stack), IR decides reality (actually can
be stack, given inlining).** Neither alone is enough: frontend-only can't see
through inlining to confirm; IR-only can't see the type-level ownership.
Together, the frontend's annotation lets you lower to `alloca` optimistically,
and `O3`+DSE either keeps it (escaped after all) or deletes it (confirmed
non-escaping).

---

## 6. Dual-compiled return forms (out-param specialization)

### The mechanism

Functions that return allocated data get two specialized forms:

```
// heap form (current)
{ i32, ptr } map(f, xs) { ... %p = malloc(n); ...; ret {n, %p} }

// caller-owned form (out-param)
void map_into(f, xs, ptr caller_buf) { ... store ... caller_buf ...; ret void }
```

The caller `alloca`s `[N x T]` once and passes `caller_buf`; no `malloc` at all.
`sroa` then dissolves `caller_buf` for the non-escaping case. The callee
signature adds one `ptr` out-param; the heap form keeps the `{i32,ptr}` return.
The specialization machinery already exists (`get_specific_callable`,
`specific_fns` cache, `function.c:503`) — this is another specialization axis
alongside the type one.

### Why it exists: it deletes the return-value root (§2, root 3)

A normal `map` must heap-allocate its result because the buffer can't live in
`map`'s frame (root 3: the return value outlives the frame). The out-param form
removes the root: the **caller** allocates (a stack `alloca`, since it's the
caller's frame and the caller's reachability decides) and passes the buffer
*in*. Then `map`'s return carries no pointer to array storage (it's `void`),
so root 3 no longer applies to the array. The caller's top-level-reachability
then decides stack-vs-heap for the buffer.

### When it's needed vs. when inlining suffices

- If `fold`/`map` always inline (tiny, and the specialize+inline stack works),
  the out-param form buys nothing — the inliner already moves the allocation
  into the caller's scope where `sroa` kills it.
- The out-param form matters for **non-inlinable** functions returning arrays
  (large DSP kernels, a `map` that didn't inline, recursive functions). It's
  the cleanup for what the inliner can't reach.

### Soundness constraint

The stack memory must be the **caller's**, allocated in the caller's frame,
with the callee only writing to it via the out-param. The signature is
`void f_into(args, ptr caller_buf)` and the callee only `store`s to
`caller_buf`. Never let the callee `alloca` and return a pointer to its own
frame — that's unsound (frame gone on return). This is the Swift `@out`
convention.

### Strategy

1. **Default to stack** (`alloca [N x T]` for const-size, `alloca T, n` for
   runtime) when none of the three roots say heap. Fixes the dead-`malloc`
   problem without dual-compilation, because inlining + `sroa` handles
   inlinable callees.
2. **Add the out-param form** as a second specialization, chosen when (a) the
   caller needs the result on the stack and (b) the callee isn't going to
   inline. Gate on `FN_ATTR_STACK_RETURN` or inferred "returns non-escaping
   allocation."
3. **Don't dual-compile everything.** Doubles code size and specialization
   cache pressure. Reserve the out-param form for functions the analysis says
   return non-escaping data *and* are too big to inline.

Do #1 first (one gate change, no new signatures, fixes most cases because most
callees inline); #2 second (targeted escalation for the non-inlined minority).

---

## 7. Copy-on-escape: the sound version of "stack-then-copy-to-heap"

### The unsound version (rejected)

"Stack-alloc by default, and if it ends up in a top-level var, heap-allocate
space and copy into it" — unsound because **you can't know at the allocation
site whether it will escape.** By the time you discover it reaches a top-level
var (possibly through 6 calls and a tuple field), the stack frame may already
be gone and the copy reads freed memory.

Two sound orderings:

**Forward (what exists): decide at allocation.** Compute reachability *before*
codegen, emit `malloc` or `alloca` once. Sound. Requires the analysis to run
first.

**Copy-on-escape (sound):** Lower to `alloca` optimistically, but emit a
guarded fixup only at the **escape edge** — the single store where the value
*becomes* reachable from a root. At that store, insert a one-time `malloc` +
`memcpy` and rewrite the root to point at the heap copy. This is Swift's
"copy-on-escape" / "stack-promotion with escape fixup."

Key differences from the unsound phrasing:
1. The escape is detected **at the store that creates the reachability**, not
   retroactively. You analyze *first* to find those stores, then emit the copy
   there.
2. The decision is still "will it escape" computed before lowering — you defer
   the *allocation choice*, not the analysis.
3. Strict improvement over "decide at allocation" only when **most allocations
   don't escape** — then you emit zero `malloc`s and pay only at the rare
   escape.

### When to adopt copy-on-escape

It's *more* codegen complexity, not less, than "decide up front": one analysis
pass *plus* escape-edge instrumentation *plus* a runtime copy. Worth it only if
the "most allocations don't escape" assumption is strong enough that copies are
rare and the win (no `malloc` for the common case) is large. The IR shows the
simpler "decide up front" path alone closes most of the gap (the dead `malloc`s
exist because the *existing* analysis isn't being applied to `@top`-local
arrays, not because the analysis is too coarse). Adopt copy-on-escape only as
a targeted fallback for genuinely "stack in common path, heap in rare path"
cases, after the simpler path is working.

---

## 8. Additional codegen optimizations (orthogonal to allocation strategy)

Observed in the `std/Arrays.ylc` IR; high-leverage and mostly low-effort.

### 8.1 Linkage and inlining (highest leverage)

Every function gets `LLVMExternalLinkage` (`function.h:15`, `codegen.c:40`).
In a single-module JIT where nothing links JIT'd functions from outside, that
disables interprocedural optimization. Switch user functions to
`LLVMInternalLinkage` (keep `ExternalLinkage` for the top-level entry and extern
decls). Unlocks the inliner, IPSCCP, and dead-arg-elim across the module; lets
`GlobalDCE` collect the dead function definitions.

Companion: `FN_ATTR_INLINE`/`FN_ATTR_NOINLINE` bits are defined (`type.h:282`)
but never emitted as LLVM attributes. Emit `alwaysinline` on freshly-specialized
generic instances (one caller → folds the specialization in, specializing type
parameters away — C++-template style).

### 8.2 Loop variable allocas defeat `mem2reg`

Loops alloca the loop variable and iterator **in the current block** (`loop.c:48`,
`:125`, `:130`, `:215`, `:217`), not the function's entry. `mem2reg` only
promotes entry-block allocas; mid-function allocas stay as stack slots. So
`for i in 0..n` runs with `i` in memory, reloads it each iteration, blocks the
vectorizer (needs the IV as an SSA phi). Fix: emit loop-carried allocas in the
**entry block**, or better, emit the loop variable as a **phi node** directly
(`%i = phi [start, entry], [%i+1, inc]`). Same for `array.c:241,321` counter
allocas and `list_eq` iterator allocas.

### 8.3 Closure env: per-use loads

`call_closure_obj` (`closures.c:57`) extracts env once. But `compile_closure`
(`closures.c:582-587`) and `compile_curried_fn` (`closures.c:204-208`) **load
each captured field fresh on every access**. Inside a hot loop touching a
captured variable N times, that's N redundant loads. GVN cleans within a block
but not across blocks (AA is conservative on `opaque`/`GENERIC_PTR`-cast env
pointers). Hoist env field loads to the **closure function's entry block**
(load once into a local, use the local thereafter) — GVN-independent,
loop-invariant by construction.

### 8.4 `strncmp` vs element-wise equality

`array_eq` (`builtin_functions.c:514`) compares two arrays via `tail call i32
@strncmp`. For two constant-filled buffers this stays runtime (libcall LLVM
won't const-fold). If it went element-wise (`_codegen_equality`), SCCP would
fold to `true` at compile time, dropping two `malloc`s + two `strncmp`s. Smaller
lever, but same class as the allocation gaps.

---

## 9. Summary table

| concern | recommendation | effort |
|---|---|---|
| array representation | keep `{i32, T*}`; use `alloca [N x T]` for const-size non-escaping buffers | low |
| size getter | keep `extractvalue` field 0; don't use `llvm.objectsize` | none |
| lifetime roots | top-level vars + coroutine state + **return value** (the third is the gap) | — |
| lifetime tracking | Layer 1 (`noescape` bit) + Layer 2 (infer for source fns) | low–med |
| pass-through returns | Layer 3 (allocation forwarding) | medium |
| where analysis lives | frontend decides intent; IR pass confirms + cleans up | — |
| non-inlinable array returns | dual-compiled out-param form (caller owns buffer) | medium |
| default strategy | stack-by-default with up-front reachability decision (NOT stack-then-copy) | low |
| copy-on-escape | only as targeted fallback, after up-front path works | medium |
| linkage | `InternalLinkage` for user functions | low |
| loop IVs | emit as phi / entry-block alloca | low–med |
| closure env loads | hoist to closure entry | low |
| array equality | element-wise instead of `strncmp` | low |

Highest-leverage first steps, in order:
1. Fix existing `escape_analysis` to classify `@top`-local arrays non-escaping
   (so `array.c:136` emits `alloca`); emit `alloca [N x T]` for const-size. →
   kills the dead `malloc`s.
2. `InternalLinkage` for user functions → collects dead definitions, unlocks
   IPO.
3. Layer 2 (infer `noescape` for source functions) → solves `test11` and all of
   `std` automatically.
4. Out-param form for non-inlinable array-returning functions → avoids heap for
   `map`/`filter` results when the caller doesn't escape them.
