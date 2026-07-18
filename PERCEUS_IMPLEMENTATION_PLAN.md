# Perceus Implementation Plan

This records the next implementation stages for Perceus-style reference
counting in MIR. Terminology should follow the Perceus paper:
`dup`, `drop`, `drop-reuse`, and reuse constructors such as `Cons@ru`.

Reference:
https://www.microsoft.com/en-us/research/wp-content/uploads/2020/11/perceus-tr-v1.pdf

## Current State

- `--perceus-rc` enables an experimental MIR pass.
- MIR has explicit `dup` and `drop` markers represented as `MIR_OP` opcodes.
- The pass currently emits `dup` for repeated consuming uses.
- Single ownership transfer is implicit; there is no `move` instruction.
- `drop` exists but automatic drop insertion is intentionally deferred.
- LLVM lowering maps the `dup` / `drop` opcodes to `__ylc_dup` /
  `__ylc_drop`.
- Runtime hooks are no-ops until heap allocation layouts carry RC headers.
- Range `AST_LOOP` now lowers through MIR as cyclic CFG blocks with `MIR_PHI`
  induction variables.

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

`dup(x)` increments the object count for heap-managed values.

`drop(x)` decrements, tests for zero, and when zero recursively drops owned
fields before freeing storage.

This needs concrete type information:

- list node: drop head and tail if managed
- array: drop each element if the element type is managed
- closure env: drop captured fields
- tuple/variant: drop contained managed fields
- string/bytes: drop backing storage according to ownership policy

Generic functions should preferably be instrumented/lowered after
specialization unless all managed values share enough uniform metadata.

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
