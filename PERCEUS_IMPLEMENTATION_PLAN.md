# Perceus Implementation Plan

This records the next implementation stages for Perceus-style reference
counting in MIR. Terminology should follow the Perceus paper:
`dup`, `drop`, `drop-reuse`, and reuse constructors such as `Cons@ru`.

Reference:
https://www.microsoft.com/en-us/research/wp-content/uploads/2020/11/perceus-tr-v1.pdf

## Current State

- `--perceus-rc` enables an experimental MIR pass.
- MIR has explicit `dup` and `drop` markers represented as `MIR_OP` opcodes.
- The pass emits `dup` for repeated consuming uses, and inserts `drop` after
  last borrowed uses, dead owned definitions, and unused owned entry params,
  with edge splitting for path-local drops. A `moved` set prevents double-drops
  of consumed (ownership-transferring) call args and post-released values.
- Single ownership transfer is implicit; there is no `move` instruction.
- LLVM lowering maps `dup` / `drop` opcodes to real runtime hooks. `dup` is a
  single uniform `__ylc_dup` that increments the rc at `payload - 8`. `drop` is
  type-specialized: a per-concrete-type `__ylc_drop_<TypeMangle>(ptr)` LLVM
  function that decrements the rc at `payload - 8`, recurses into owned
  children at rc == 0, and frees. Stack-allocated managed values carry an
  `rc == 0` sentinel header so `dup`/`drop` no-op on them safely.
- Escape analysis marks consumed call args (and their alloc-site fields) as
  escaping so locally-constructed values passed to a callee that drops them are
  heap-allocated with a real RC header.
- Range `AST_LOOP` now lowers through MIR as cyclic CFG blocks with `MIR_PHI`
  induction variables.
- `tag_or_size_class` is the second 32-bit slot of `YlcRcHeader`. It was
  historically reserved (always written `0`, never read). It is per-allocation
  runtime metadata for the type-specialized drop and reuse: for arrays it holds
  the element count (so a payload-only drop fn can iterate elements), for list
  cons it is unused (fixed two-field node), for closure env it holds the field
  count. It is distinct from the compile-time type/shape descriptor used for
  reuse compatibility, which is determined post-specialization and does not
  need a runtime slot.

## Stage 1: Operand Ownership Modes

Status: implemented as `MirOperandUse` metadata exposed by
`mir_instr_for_each_operand` / `mir_term_for_each_operand`.

Ownership mode metadata belongs on MIR operands, not only on function
parameters.

Suggested model:

```c
typedef enum {
  MIR_OPERAND_BORROW,
  MIR_OPERAND_OWN,
} MirOperandMode;

typedef struct {
  MirValueId value;
  MirOperandMode mode;
} MirOperand;
```

Examples:

```mir
%n = array_size borrow %arr
%t = construct.tuple { own %xs, borrow %n }
%r = call %f(own %arg, borrow %cfg)
return own %r
```

Rules of thumb:

- `array_size`, tag tests, primitive comparisons: borrow.
- `array_fill` borrows its fill function; it calls the function but does not
  store the callable in the result.
- `construct.tuple`, `construct.variant`, `construct.closure_env`,
  `construct.array_literal`, `construct.list_cons`: own stored managed values.
- `return`: own returned managed value.
- extraction operations (`extract.array_at`, `extract.field`, `extract.list_head`) usually borrow from the parent unless explicitly duplicated.
- call operands follow the callee ownership summary.

## Stage 2: Function Ownership Summaries

Status: implemented as `MirFnSummary` for function declarations, builtins, and
callable MIR values.

Function and builtin summaries should make calls instrumentable without
guessing from textual use counts.

```c
typedef enum {
  MIR_PARAM_BORROWED,
  MIR_PARAM_OWNED,
} MirParamConvention;

typedef struct {
  MirParamConvention *params;
  size_t param_count;
  bool returns_owned;
} MirFnOwnershipSummary;
```

Use this for:

- direct function calls: implemented through `MirFunction.summary`
- specialized generic calls: implemented by cloning source summaries onto
  specialized functions
- closure calls: the closure environment operand is currently pushed as an
  explicit borrow at the call site
- builtin calls: implemented through `MirBuiltinSymbol.summary`
- first-class function values: implemented through `MirValue.callable_summary`
  on `fn_ref`, closure construction, `extract.closure_fn`, and callable-typed
  values such as higher-order parameters

Function parameters can store a default convention, but call lowering should
consult the callable summary because closure/generic/specialized calls may not
have a direct `MirFunction` available at the call site. Current summaries are
still conservative defaults inferred from function type shape unless the callee
is a builtin with an explicit summary.

## Stage 3: CFG Liveness And Safe Drop Insertion

Status: started. The MIR Perceus pass now computes block-level `live_in` /
`live_out`, uses block-local remaining counts for `dup`, emits conservative
`drop` markers after last borrowed uses, unused owned definitions, and unused
owned entry parameters, and splits conditional edges for path-local drops.
Borrowed extraction results keep their source owner live through
extraction-result uses. Projection results rooted in a stack-allocated parent
are treated as non-RC-managed, so stack container extractions do not receive
heap `dup` / `drop` markers. Match results now lower to continuation-block
`MIR_PHI` values, so unused owned match results are handled as ordinary unused
owned definitions. `MIR_PHI` operands are represented as predecessor-edge
liveness requirements rather than ordinary header-block uses.
Managed `array_set` now emits an old-slot load and `drop` around the store, so
overwritten managed elements have an explicit release point in MIR.

Do not insert `drop` from textual use counts. Correct placement needs CFG
liveness:

- branch and match arms
- join blocks
- loops
- early returns
- tail calls
- borrowed extractions

Borrowed extractions are the immediate hazard:

```mir
%env = extract.closure_env %closure
%fn  = extract.closure_fn %closure
%r   = call %fn(%env, ...)
```

Captured closure fields and future coroutine state fields should use generic
`extract.field` extractions. Whether the source lowers as an SSA struct, stack
slot, heap environment, or coroutine frame pointer is an LLVM-lowering detail.

`%closure` cannot be dropped immediately after `%env` / `%fn` are extracted if
those values borrow from it. The drop belongs after the last use of the borrowed
extraction.

Remaining work:

- Managed-value phi ownership now has dedicated stress tests in
  `test/mir_scripts/perceus_pipeline.ylc` covering pre-existing owned values
  selected into a non-induction `MIR_PHI`, with consume, duplicate, and
  borrow-then-return uses.
- Managed-value loop-carried state is deferred for now. Range-loop MIR
  currently only emits the integer induction `MIR_PHI`, and we are not adding
  more complicated non-induction loop fixtures at this stage.
- Runtime `drop` hooks are still no-ops, so old-slot release is visible in MIR
  but not yet backed by real RC decrement/free behavior.

## Stage 4: Runtime Object Layout

Heap allocations need a consistent RC header. Decide whether MIR values point to
the payload or the header, and make all heap allocations follow the same rule.

Candidate:

```c
typedef struct YlcRcHeader {
  uint32_t rc;
  uint32_t tag_or_size_class;
} YlcRcHeader;
```

Update allocation lowering for:

- list nodes: MIR-to-LLVM lowering now allocates `{ YlcRcHeader, payload }`,
  initializes `rc = 1`, and returns the payload pointer as the list value.
- arrays: MIR-to-LLVM lowering now allocates `{ YlcRcHeader, payload }`,
  initializes `rc = 1`, and stores the payload pointer in the array struct data
  field.
- closure envs: MIR-to-LLVM lowering now allocates `{ YlcRcHeader, payload }`,
  initializes `rc = 1`, and stores the payload pointer in the closure pair.
- strings: still pending; MIR string constants are not lowered through the
  current MIR-to-LLVM path yet.
- boxed/sum payloads: no separate MIR heap allocation path is active yet; revisit
  if variants start boxing payload storage independently.

## Stage 5: Type-Specialized Dup/Drop Lowering

Status: implemented for list cons, arrays, and closure envs.

`dup(x)` increments the object count for heap-managed values.

`drop(x)` decrements, tests for zero, and when zero recursively drops owned
fields before freeing storage.

This needs concrete type information:

- list node: drop head and tail if managed — implemented via `llnode_type`
  element/tail GEPs and recursive child drop.
- array: drop each element if the element type is managed — **deferred**; the
  `__ylc_drop_Array_<T>` body currently frees without iterating elements. Needs
  `tag_or_size_class` to hold the element count (see Stage 6 step 1).
- closure env: drop captured fields — implemented by recursing over
  `closure_meta` env field types.
- tuple/variant: drop contained managed fields — not yet exercised (boxed/sum
  payloads do not allocate independently).
- string/bytes: drop backing storage according to ownership policy —
  `is_string_type` values use a string layout; backing-storage drop is not yet
  wired through the MIR path.

Generic functions are instrumented after specialization, so concrete types are
available at lowering time; a per-type drop function is feasible and
implemented. Mangled drop names are sanitized to valid LLVM identifiers
(`__ylc_drop_<sanitized>`). Coroutines are excluded from RC management
(deferred; coroutine frames use a distinct allocation path without an RC
header).

## Stage 6: Reuse Analysis

After correct `dup`/`drop`, add Perceus reuse.

Shape:

```mir
%ru = drop-reuse %old
%new = construct.list_cons@ru %ru, own %head, own %tail
```

Requirements:

- uniqueness check at `drop-reuse`
- reusable allocation token
- constructor/allocation forms that consume reuse tokens
- same-size/same-shape compatibility checks
- fallback allocation path when the value is not unique

Start with list cons and simple same-shaped variants before arrays.

### Step 1: Header shape/size metadata + array recursive drop

Status: implemented.

Populate `tag_or_size_class` at allocation time so a drop/reuse knows the
allocation shape, and close the deferred array element-drop so array-of-list
reuse does not leak old elements.

Decision: the array element count is stored in `tag_or_size_class` (the
allocation capacity), not passed alongside the payload. The drop fn is called
on the allocation base (`data - offset`); a slice shares the backing store with
a smaller `len`, so the slice `len` would under-count. `len + offset` would be
correct but is avoided to keep the uniform `drop(ptr)` ABI. The header
capacity is the authoritative count for freeing; the array struct's `len` is
the user-visible (slice) length and duplicates the count for the offset-0
owner, which is the accepted cost of the uniform payload-pointer drop ABI.

- list cons: `tag_or_size_class = 0` (fixed two-field node; no count needed).
- arrays: `tag_or_size_class = element_count` at `lower_mir_heap_alloc_payload`
  / `lower_mir_stack_alloc_payload`, read from `construct.array_literal` items
  length (and `array_fill` size). The count lives in the header (not the array
  struct's user-visible `len`), because the type-specialized drop fn receives
  only the payload pointer. This also unblocks the Stage 5 array element-drop
  TODO.
- closure env: `tag_or_size_class = field_count` (for shape checks; env drop
  already recurses via `closure_meta`).
- Implement the array element-drop loop in `lower_mir_build_drop_fn_body` for
  `is_array_type`: load `tag_or_size_class` from the header, iterate
  `[0, count)`, derive each element's managed payload pointer (bare `ptr` for
  list/closure element types via the same derivation as
  `lower_mir_drop_load_child_ptr`), and call the element type's drop fn.
  Verify: `std/HashSet` (array of lists) under ASAN, `13_array_sum`.

### Step 2: `MIR_OP_KIND_DROP_REUSE` opcode + lowering

Status: implemented (opcode + lowering; emission deferred to step 4).

Add a reuse drop opcode whose lowering yields a reuse token when the value is
uniquely owned, and falls back to a normal drop otherwise.

- `mir.h`: add `MIR_OP_KIND_DROP_REUSE` to the `MirOpKind` enum near
  `MIR_OP_KIND_DROP`. It carries one operand (the dropped value) and its
  result is the reuse token (a payload pointer, or null).
- `mir.c`: dump it as `drop-reuse %v`.
- `lowering.c`: lower `MIR_OP_KIND_DROP_REUSE` to a small inline sequence:
  compute `header = ptr - 8`; `load rc`; if `rc == 1`, store `rc = 1` (reset
  for reuse), yield `ptr` as the reuse token (the constructor will overwrite
  the payload in place); else call the type-specialized drop (as `DROP` does
  today) and yield null. The token type is the generic pointer.
- Keep `MIR_OP_KIND_DROP` lowering unchanged; `DROP_REUSE` is a superset used
  only where the reuse analysis (step 4) rewrites a paired drop.

### Step 3: Reuse-aware constructors

Status: implemented for list cons.

Make `construct.list_cons` (and later `array_literal`/`closure_env`) accept an
optional reuse token.

- `MirConstruct` gained a `reuse_token` field (`MirValueId`, default
  `MIR_NO_VALUE`). All construct emit helpers initialize it to `MIR_NO_VALUE`.
  The operand visitor and operand rewriter for `MIR_CONSTRUCT_LIST_CONS`
  include the token (borrowed) when present, and MIR dumps it as
  `construct.list_cons@ru %tok, %head, %tail`.
- `lower_mir_list_cons`: if a token is present, load it; if non-null, use it as
  the payload slot (overwrite in place, rc already 1); if null or absent,
  `lower_mir_heap_alloc_payload` a fresh node. Same-shape check is static
  post-specialization (the reused node's element type equals the cons element
  type by construction).
- The runtime representation is unchanged: reuse is instrumentation around the
  site, replacing a `free` + `malloc` pair with an in-place overwrite gated by
  a null check on the token.

### Step 4: Perceus reuse pairing analysis

Status: implemented for list cons.

In `mir_perceus.c`, pair a `drop %old` with a same-block `construct.list_cons`
of a new same-shape value, rewriting the `drop` into `drop-reuse` feeding the
constructor.

- `mir_perceus_pair_reuse` runs per-block after liveness/drop instrumentation.
  For each `construct.list_cons` it scans the whole block for a same-shape
  `drop` of a list value of the same element type (post-specialization, so
  element-type pointer equality is the shape check). If the dropped value is
  not used between the drop and the cons, it removes the plain `drop`, emits a
  `drop-reuse` (yielding a `t_ptr` token) just before the cons, and sets the
  cons's `reuse_token`. The cons's lowering then overwrites the recycled slot
  in place when the token is non-null, falling back to a fresh `malloc` when
  the value was not unique (rc != 1).
- Verified on `scratch/reuse_demo.ylc` (list reversal): MIR emits
  `drop-reuse %0` + `construct.list_cons@ru %tok, %head, %acc`, the LLVM IR
  shows the `rc == 1` uniqueness check + `select` reuse path, and the program
  passes under ASAN. All existing tests (289/289 MIR, test_scripts, std
  HashSet/Lists) remain green.

### Reuse scope: exact shape & size only

Reuse is restricted to **exact-shape** recycling for now: the reused allocation
must hold the same element type and the same (or smaller, same-type) count as the
new value. This keeps the shape check a simple equality (lists: element-type
pointer equality post-specialization; arrays: element type + `tag_or_size_class`
count) and the token a plain `null`/`non-null` payload pointer.

Deferred (not in scope now):
- **Cross-type size-class reuse** (e.g. `Array of Int` (4B×10) → `Array of
  Double` (8B×5), same total bytes). Valid when `sizeof(new) * count(new) ≤
  sizeof(old) * count(old)` and alignment holds, but requires a runtime
  size+alignment check carried in the token and, critically, an explicit drop
  of the old managed elements before overwrite (else they leak). Keep as a
  follow-up.
- **Smaller-count same-type reuse** of *managed-element* arrays: needs the
  reuse path to drop the excess old elements (indices `new_count..old_count`)
  before overwriting, since the type-specialized drop only iterates the new
  count. Same-type smaller-count for *primitive-element* arrays is safe without
  that guard and could land first.

Arrays (planned): `MIR_CONSTRUCT_ARRAY_LITERAL` gains a `reuse_token`; the
constructor overwrites the recycled backing store element-by-element when the
token is non-null, and the pairing analysis extends to array constructs with a
runtime count check (`old.tag_or_size_class == new_count`). Reuse fires when a
consuming operation (e.g. an in-place-style `map` of the same element type and
length) drops the input array and builds a same-shape output — a type-changing
map (`Int -> Double`) or different-length result falls back to fresh `malloc`.

Closure envs (planned, lowest priority): `MIR_CONSTRUCT_CLOSURE_ENV` gains a
`reuse_token`; shape check is env field count + field types. Applies when a
closure is rebuilt with the same env shape.

### Step 5: Tests

Add `scratch/` reuse fixtures (see Stage 7) asserting `drop-reuse` + `@ru`
constructs appear in MIR, and that reuse programs run under ASAN without
leaks/double-frees. The existing `list_rev`/`list_map` programs are the first
reuse targets; compare their MIR before/after.

## Stage 7: Tests And Galleries

Keep focused scripts under `scratch/`:

- duplicate array into tuple: should show `dup`
- move-only return: should show no `dup`
- borrowed array size/indexing: should not `dup`
- extracted stack container element: should not `dup`
- branch with returned heap array: should show path-local edge `drop`
- unused heap match result: should drop in the match continuation block
- tail-recursive managed value transfer: should not insert a premature `drop`
  or path-insensitive `dup`
- range loop borrowing a heap array across the backedge: should show `MIR_PHI`
  for the induction variable and no premature `drop` of the array
- range loop replacing a managed array cell: should keep the mutable container
  live through the loop, avoid dropping the borrowed `array_set` alias, and
  eventually cover old-slot release once runtime RC hooks are real
- closure env extraction: must not drop closure before env/function use ends
- match branches: drops must be branch-local or placed after joins correctly
- tail recursion: drops must not break tail-call lowering

Useful commands:

```sh
./build/ylc --perceus-rc --dump-mir scratch/mir_perceus_gallery.ylc
./build/ylc --perceus-rc --verify-ir scratch/mir_perceus_gallery.ylc
```
