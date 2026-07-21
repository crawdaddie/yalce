# MIR Function Identity Plan (MirFunctionId -> MirFunction *)

Status: design plan + audit. Not yet implemented. This is the prerequisite for
persisting top-level MIR function bodies across REPL inputs (see the "Persisting
MIR (not AST)" direction in the REPL persistence work).

## Why

The native (`orcjit`) and wasm REPLs now thread a persistent, malloc-backed
top-level MIR scope across compile calls, so value globals persist via
`MIR_SYMBOL_GLOBAL` -> `global_load`. But top-level functions and generic
functions do not yet persist, because function identity is currently a
`MirFunctionId` — an index into the **current** `MirProgram->functions`
vector. A function defined in input N is invisible to input N+1: the id is
stale, and the function body lives in input N's (destroyed) program arena.

Persisting the MIR function bodies themselves (not re-lowering from the AST)
is the chosen direction. For that to be sound, a durable `MirFunction *` must
be self-consistent regardless of which `MirProgram` it's lowered in: every
function reference must resolve to the same function by **pointer identity**,
not by a program-relative id.

The blocker is that `MIR_FN_REF`, `MIR_CALL.specialized_fn`, `MirSymbol.as.function`,
and `MirFunction.specialization_of` all carry `MirFunctionId`. Changing them to
`MirFunction *` makes durable function bodies correct in any program, because a
durable body's internal references point at durable `MirFunction *`s directly.

## Current behavior (the bug this fixes)

```
$ ylc --dump-mir
λ let f = fn x -> x + 1;;
# ylc-mir
fn $top() -> Int @owned {
  bb0 entry:
    %0 = fn_ref $f
    return %0 ; ops [%0:return/consume#0]
}
fn f(%0 x: Int @consume) -> Int @owned { ... }

λ f
MIR Error: unresolved identifier `f` <repl:1> 1:1
```

Input 2 references `f`, but the durable `MIR_SYMBOL_FUNCTION` for `f` carries
input 1's program-relative id, which the generation guard (`mir_ctx_lookup_symbol`
in `lang/mir/mir.c`) correctly skips as stale — and there is no fallback that
re-resolves the durable function into the current program.

## What already landed (context for this session)

These changes are already on the branch and should not be reverted:

- `lang/mir/mir.h`: `MirSymbol` and `MirProgram` gained a `generation` field.
- `lang/mir/mir.c`:
  - `mir_current_generation` static counter, bumped at the top of
    `mir_build_program`; `mir_symbol_new` stamps `symbol->generation`.
  - `mir_stack_frame_init(NULL, ...)` uses a malloc-backed `ht` allocator
    (`mir_ht_alloc_persistent` / `mir_ht_free_persistent`).
  - `mir_symbol_new(NULL, ...)` uses `malloc`.
  - `mir_scope_arena(ctx)` resolves the allocator from the current frame's
    `ht` (NULL = persistent root, program arena otherwise). All top-level
    `mir_symbol_new` call sites use it.
  - `mir_ctx_lookup_value` skips stale `MIR_SYMBOL_VALUE` (generation mismatch).
  - `mir_ctx_lookup_symbol` skips stale `MIR_SYMBOL_VALUE` / `MIR_SYMBOL_FUNCTION`.
  - `mir_export_expr_binding` `strdup`s `global_name` into malloc when the scope
    arena is persistent (NULL), so `MIR_SYMBOL_GLOBAL` names survive.
  - `mir_let` skips the `mir_bind_pattern` value bind at top level when a durable
    binding was just recorded (`durable_bound`), so the value bind doesn't
    clobber the durable `MIR_SYMBOL_GLOBAL`. NOTE: this gate is currently
    `expr->tag != AST_LAMBDA`, i.e. it does NOT yet protect functions — functions
    still get a `MIR_SYMBOL_VALUE`? No: functions go through `mir_bind_function_symbol`
    (FUNCTION), not the value path. The gate is correct for values; functions
    need the pointer-identity work below.
- `lang/backend_llvm/orc.c`: `orcjit` allocates a persistent `MirStackFrame`
  (`mir_stack_frame_init(NULL, ...)`) and threads it through `compile_script` ->
  `compile_source` -> `MirCtx.frame`. `mir_build_program` reuses it.
- `wasm/main.c` + `wasm/Makefile`: wasm backend scaffolding (not yet wired to
  the persistent frame; the orcjit path is the reference implementation).
- `test/test_repl_persistence.sh`: 24 checks; value globals + accumulation
  pass, functions/generics fail pending this work.
- `lang/format_utils.h`: `YLC_NO_COLOR` strips ANSI escapes (wasm build only).

## The change: MirFunctionId -> MirFunction *

### Storage fields to change (`lang/mir/mir.h`)

1. `MirInstr.data.fn_ref.fn`: `MirFunctionId` -> `MirFunction *`.
   - `MIR_NO_FUNCTION` sentinel becomes `NULL`.
2. `MirInstr.data.call.specialized_fn`: `MirFunctionId` -> `MirFunction *`.
   - `MIR_NO_FUNCTION` sentinel becomes `NULL`.
3. `MirSymbol.as.function`: `MirFunctionId` -> `MirFunction *` (in the union,
   alongside `value` / `module` / `expr` / `global_name`). The union already
   holds pointer-sized members, so this is layout-compatible.
4. `MirFunction.specialization_of`: `MirFunctionId` -> `MirFunction *`.

`MIR_NO_FUNCTION` is currently `(MirFunctionId)-1`. With pointers, use `NULL`.
Audit call sites that compare against `MIR_NO_FUNCTION` (see audit below) and
convert them to `NULL` comparisons.

### Read sites to update (id-lookup -> pointer-deref)

#### `lang/llvm/lowering.c` (LLVM lowering resolves refs via id-keyed arrays)
The lowering keeps `lctx->functions[id]` and `lctx->function_types[id]`
(`MirLlvmCtx`), arrays indexed by `MirFunctionId`. After the change, refs carry
`MirFunction *`, so these arrays must become keyed by `MirFunction *`. The
cleanest: keep a per-program id map for **declaration order** (the lowering
still declares functions in `program->functions` order), but resolve
`MIR_FN_REF` / `MIR_CALL.specialized_fn` by looking up the declared LLVM value
through a `MirFunction * -> LLVMValueRef` map instead of the id array.

Specific sites (all currently `lctx->functions[...]` / `lctx->program->functions.items[id]`):
- 829-837: `lower_mir_fn_ref` — reads `instr->data.fn_ref.fn` as id, indexes
  `lctx->functions` and `program->functions.items`.
- 3029-3033: `lower_mir_call` specialized_fn path.
- 3037-3050: `lower_mir_call` callee-via-fn_ref path (`callee_def->data.fn_ref.fn`).
- 3441-3444, 3456-3475: `lower_mir_call_value` / coroutine call specialized + callee paths.

#### `lang/mir/mir.c` (MIR build/resolution)
`mir_program_get_function(builder->program, <id>)` calls on function refs:
- 1224, 2789, 2999, 3050, 3601, 3688, 3994, 4930.

These become direct pointer derefs: `instr->data.fn_ref.fn` *is* the function.

`MirSymbol.as.function` reads:
- 4002, 4026, 5584: `mir_program_get_function(builder->program, symbol->as.function)`
  -> `symbol->as.function` directly.

`MIR_CALL.specialized_fn` reads/writes:
- 3675 (read), 3476, 3488, 4075, 4133 (write `specialized->id` -> `specialized`).

`specialization_of`:
- 827-828 (`mir_program_find_specialization` matches `fn->specialization_of == source_id`).
- 854-855 (init to `MIR_NO_FUNCTION` -> `NULL`).
- 3372-3373 (set `fn->specialization_of = source->id` -> `source`).

#### `lang/mir/mir_escape.c` (escape analysis)
- 187-203: `call->data.call.specialized_fn` and `callee->data.fn_ref.fn` resolved
  via `mir_program_get_function`. Become pointer derefs.
- 358: `mir_escape_mark(state->escaped, fn, instr->data.call.callee)` — `callee`
  is a `MirValueId` (unchanged; see "not changed" below).

### Write sites to update

- `lang/mir/mir.c:2285-2286`: `instr.data.fn_ref.fn = fn->id; instr.data.fn_ref.name = fn->name;`
  -> `instr.data.fn_ref.fn = fn;` (keep `name` for dumps).
- `lang/mir/mir.c:4914, 4954`: `symbol->as.function = fn->id` / `target->id` -> the pointers.
- `mir_program_add_function` (`mir.c`): `fn->id` stays (for per-program declaration
  order / lowering bookkeeping), but `fn` is the durable identity.

### Find-function helpers (must also scan durable functions)

- `mir_program_find_function_by_name` (`mir.c`): scans `program->functions`.
  Must also find durable functions that aren't (yet) in this program's
  `functions`. Simplest approach: when building, **register durable functions
  into the current `program->functions`** (append the durable `MirFunction *`,
  assigning a fresh per-program id for declaration order). Then
  `program->functions` scans find them. References resolve by pointer, so the
  per-program id is only for the lowering's declaration iteration, not for
  reference resolution. Internal refs inside a durable body stay correct
  because they're pointers.
- `mir_program_find_specialization` (`mir.c:827`): matches
  `fn->specialization_of == source_id`. With pointers: `== source_ptr`. Same
  scan; durable specializations are in `program->functions` once registered.

### `mir_value_fn_ref_target` (`mir.c:4924`)

Resolves a `MirValueId` (a `MIR_FN_REF` value) to its `MirFunction`. It does
`mir_function_find_def_instr(fn, value)` (value-id lookup, unchanged) then
`mir_program_get_function(program, instr->data.fn_ref.fn)`. The last step
becomes `instr->data.fn_ref.fn` (the pointer directly).

### `program->functions` vector — declaration/iteration, not identity

`lower_mir` (`lowering.c:4993, 5011`) iterates `program->functions` to declare
and lower each function. This stays. The key invariant after the change:
**durable top-level functions must be present in the current program's
`functions` vector** so they get declared/lowered, even though references to
them resolve by pointer. The build registers them (see find-function helpers
above). The per-program `fn->id` is just the declaration index.

## What is NOT changed

- `MIR_CALL.callee`: a `MirValueId` pointing at a `MIR_FN_REF` instruction
  via `mir_function_find_def_instr`. This is a **within-function value
  reference** — it's fine for persistence because the whole function body
  (including its `MIR_FN_REF` instrs) persists together. The `MIR_FN_REF`
  it points at now stores a pointer; the value-id lookup path is unchanged.
  Sites: `mir.c` 2772, 3092, 3434, 3601, 3682, 3684, 3688; `mir_escape.c:196`;
  `lowering.c` 3042, 3330, 3334, 3358, 3362, 3460, 3464.
- `mir_function_find_def_instr` / `mir_function_value_type`: operate on a
  `MirFunction *` and `MirValueId`; unchanged.
- `MirValueId` / `MirBlockId`: within-function ids; unchanged (function bodies
  persist as a unit, so internal ids stay consistent).
- The `generation` mechanism: stays. It still guards stale `MIR_SYMBOL_VALUE`.
  For `MIR_SYMBOL_FUNCTION`, once the symbol carries a `MirFunction *` (durable
  pointer), the generation guard for FUNCTION should be **removed** (functions
  are now durable and resolvable by pointer). Keep the guard for VALUE only.

## Durable function allocation (the persistence half)

Separate from the pointer-identity change but required to actually persist:

- A persistent `MirArena` held in the REPL session (`orcjit`'s session, and
  `wasm/main.c`'s `g_session`). Top-level functions (`mir_lambda_value` at top
  level) allocate their `MirFunction`, blocks, instrs, values, params, and
  names from this persistent arena, and `fn->arena` points to it.
- The durable `MIR_SYMBOL_FUNCTION` (carrying the `MirFunction *`) is bound
  into the persistent root frame.
- At build time, `mir_ctx_lookup_symbol` finds the durable symbol; the build
  registers the durable `MirFunction *` into the current `program->functions`
  (fresh per-program id, for declaration order) and emits `MIR_FN_REF` with
  the pointer. The lowering reads the durable body via the pointer.

The pointer-identity change (above) is what makes the durable body correct:
internal references in the durable body (recursive calls, calls to sibling
top-level functions) are pointers, so they resolve correctly in any program.

## Closures (deferred)

A non-capturing top-level function (`let f = fn x -> x + 1`) is the clean case.
A closure (`let adder = fn x -> fn y -> x + y`) captures values from the
enclosing scope; the closure *value* (`mir_closure` construct) references the
function pointer + an env built from current-program value ids. The env
values are program-relative and break persistence. Plan: persist
non-capturing top-level functions first; defer closures (they'll resolve
within their defining program; cross-input closure use is a follow-up).

The closure detection: `is_closure(expr->type)` / `expr->type->closure_meta`
(see `mir_lambda_value` at `mir.c:5025-5050`). Non-capturing functions return
at line 5031 (early); closures continue to build the env. Gate durable
allocation on the early-return path (no `closure_meta`).

## Generics

Generic functions specialize per call site. With `specialization_of` as a
pointer, `mir_program_find_specialization` matches by pointer + type. Durable
specializations persist the same way (durable `MirFunction *`, registered into
each program). The specialization machinery (`mir.c:3372`, `mir_specialize_fn_ref_instr`)
writes `specialization_of = source` (pointer) and `specialized_fn = specialized`
(pointer). No additional mechanism beyond the pointer-identity change +
durable allocation.

## Risks

1. **Lowering id-keyed arrays**: the largest mechanical change. `lctx->functions`
   / `lctx->function_types` (indexed by id) must become `MirFunction *`-keyed.
   A small hash map or a parallel `MirFunction **` lookup is needed. Keep the
   per-program id only for declaration order in `lower_mir`'s iteration.
2. **`MIR_NO_FUNCTION` -> `NULL`**: audit all `!= MIR_NO_FUNCTION` / `== MIR_NO_FUNCTION`
   comparisons on the changed fields and convert to `NULL`. Sites: `mir.c`
   1184, 2991, 3172, 3673, 3684, 4129, 4190, 4927; `mir_escape.c` 187, 198;
   `lowering.c` 3029, 3037, 3241, 3330, 3358, 3441, 3456.
3. **Dump**: the MIR text dump prints `fn_ref $name` (by name, not id) —
   `instr.data.fn_ref.name` is retained, so dumps are unaffected. Verify no
   dump site reads `.fn` as an id for printing.
4. **`program->functions` registration**: durable functions must be registered
   into each new program before the build resolves refs to them. The
   registration point is the start of `mir_build_program` (after the persistent
   frame is attached) or lazily on first reference. Eager registration at
   `mir_build_program` start is simpler and matches the existing "functions
   vector is the declaration list" model.

## Implementation order (suggested)

1. Change the 4 storage fields to `MirFunction *` (or `NULL` sentinel) in
   `mir.h`. Update `MIR_NO_FUNCTION` comparisons.
2. Update write sites (`mir.c:2285`, `4914`, `4954`, `3372`) to store pointers.
3. Update read sites in `mir.c` + `mir_escape.c`: `mir_program_get_function(program, id)`
   -> direct pointer deref.
4. Update `mir_value_fn_ref_target`, `mir_program_find_specialization`,
   `mir_program_find_function_by_name` for pointer identity + durable scan.
5. Update the LLVM lowering (`lowering.c`): `lctx->functions`/`function_types`
   id-keyed arrays -> `MirFunction *`-keyed; update all ~7 resolution sites.
6. Add durable allocation: persistent `MirArena` in the REPL session;
   `mir_lambda_value` top-level path allocates from it; register durable
   functions into `program->functions` at `mir_build_program` start.
7. Remove the `MIR_SYMBOL_FUNCTION` generation guard (functions now durable by
   pointer); keep `MIR_SYMBOL_VALUE` guard.
8. Wire `wasm/main.c`'s `g_session` to hold the persistent frame + arena
   (mirror `orcjit`).
9. Run `test/test_repl_persistence.sh` (gate: 24/24) + `test/test_mir_pipeline.sh`
   (288/288) + `make -C test test_scripts` (no regressions).

## Test gate

- `bash test/test_repl_persistence.sh` — 24 checks. Functions/generics
  sections flip from fail to pass. Currently 14/24 fail (value globals pass,
  functions/generics fail pending this work).
- `bash test/test_mir_pipeline.sh` — 288/288 (must not regress).
- `make -C test test_scripts` — all pass (must not regress).
- The user's reported case: `let x = 2` then `x` emits `global_load @$global.x`
  (already fixed). After this work: `let f = fn x -> x + 1;;` then `f 41`
  resolves `f` across inputs (no "unresolved identifier", lowers correctly).
