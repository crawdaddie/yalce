# Readability Review: `lang/backend_llvm/`

Read-only review pass. No source changed. Findings are concrete, with file:line
references and the *why* each is hard to follow. Proposed fixes lean toward
directness and flat switches over indirection/nesting.

The highest-impact improvements are #1 and #5 (coroutine boilerplate and the
coroutine drain-loop idiom), followed by #2 (codegen_fn_type duplication) and
#3 (generic-fn specialization dispatch).

---

## 1. Coroutine wrapper handlers: ~150 lines of identical setup/cleanup boilerplate per handler

`coroutines/coroutine_extensions.c` (3596 lines) is by far the most complex
file. Almost every `Cor*Handler` (CorLoop, CorMap, CorFilter, CorTake,
CorOfCorList, CorOfList, CorOfArray, CorZip, CorZipStruct, ...) hand-emits the
same 4-phase coroutine skeleton inline:

```tsv
phase	what it emits	occurrence count
entry: promise alloca + is_done=0 + reset/args null	PROMISE_SET_RESET_FN(...ConstNull...) block	7x ext + 1x coro.c
entry: coro.id / coro.size / malloc / coro.begin	the 4-call setup	27x coro.id, 17x in coroutines.c
initial suspend switch	initial.save / initial.suspend + switch cases	31x initial.suspend
final suspend switch	final.save / final.suspend + switch cases	27x
cleanup/suspend tail	coro.free + free + coro.end + Ret handle	27x
```

Concrete examples (all in `coroutine_extensions.c`):

- `CorMapHandler` lines 423-484 (entry/setup) and 617-636 (cleanup/suspend) are
  byte-for-byte the same as `CorFilterHandler` lines 711-772 / 916-934, and
  `CorTakeHandler` lines 1086-1160 / 1301-1320. The only handler-specific bit
  is the loop body in the middle.

- `CorZipHandler` repeats the exact same entry/setup at 2950-3007 and the same
  cleanup/suspend at 3181-3196. `CorZipStructHandler` repeats it again at
  3329-3380 / 3568-3581.

- The `coro_emit_setup` helper (coroutines.c:1284-1317) and
  `coro_emit_initial_suspend`/`coro_emit_final_suspend`/
  `coro_emit_cleanup_and_suspend` (coroutines.c:1323-1495) **already exist and
  are used by `compile_coroutine`** — but the `Cor*Handler` wrappers were
  written before/independently and do not call them. So the helpers are
  reimplemented inline ~10 times.

**Why it's hard to follow:** a reader scanning `CorMapHandler` must mentally
skip ~130 lines of boilerplate before reaching the actual map logic
(`get_value_bb` at line 535). The same "skip the prologue/epilogue" tax applies
to every handler, and the boilerplate is *slightly* different each time (some
write frame size to an out-param, some don't; some take a closure env, some
don't), so you can't trust copy-paste — you must re-read it each time.

**Proposed fix (direct, flat):** introduce one helper that emits the full
prologue+epilogue and returns the handle + start/cleanup/suspend blocks:

```c
typedef struct {
  LLVMValueRef handle, id, promise_alloca;
  LLVMTypeRef promise_type;
  LLVMBasicBlockRef start_bb, cleanup_bb, suspend_bb;
} CoroFrame;

// Emits entry (promise/id/begin/initial-suspend), leaves builder at start_bb,
// and pre-emits cleanup_bb+suspend_bb so the handler body can `LLVMBuildBr`
// to cleanup_bb/suspend_bb freely. Restores builder to a caller block.
CoroFrame coro_begin_wrapper(LLVMModuleRef, LLVMTypeRef llvm_yield_type,
                             bool write_frame_size_out, LLVMValueRef fn, ...);
```

Each `Cor*Handler` then becomes: `CoroFrame f = coro_begin_wrapper(...);`
<emit the handler-specific loop using f.start_bb/cleanup_bb/suspend_bb>
`coro_finish_wrapper(&f);` (emits final suspend + cleanup + suspend + ret,
restores builder). This collapses each handler by ~130 lines and makes the
handler-specific logic the *only* thing in the function body.

---

## 2. `codegen_fn_type` vs `codegen_coro_fn_type`: near-duplicate parameter-type lowering

`function.c:20-94` (`codegen_fn_type`) and `coroutines.c:805-874`
(`codegen_coro_fn_type`) are the same algorithm with one difference: the
coroutine version prepends an `i64* frame_size_out` parameter. Both share:

- identical `T_VOID`-from early return (function.c:34-41 vs coroutines.c:813-821),
- the same `for (f = fn_type; f->kind == T_FN && !is_closure(f); ...)` loop,
- the *same 5-way `if/else if` ladder* on the param type
  (function.c:47-79 vs coroutines.c:827-859):
  ```c
  if (is_closure(t))        -> type_to_llvm_type(t)
  else if (t->kind==T_FN)   -> GENERIC_PTR
  else if (ptr && num==0)   -> GENERIC_PTR
  else if (ptr)             -> LLVMPointerType(inner, 0)
  else if (coroutine_type)  -> GENERIC_PTR
  else                      -> type_to_llvm_type(t)
  ```
  (function.c omits the bare `T_FN` arm in favour of an `is_closure` check;
   coroutines.c folds `T_FN` and non-closure under `GENERIC_PTR`. Subtle.)
- the same NULL-check + return-type handling (function.c:81-94 vs 861-874).

**Why it's hard to follow:** two readers reasoning about "how is a function
parameter lowered to an LLVM type" must consult both files and reconcile the
two slightly-different ladders. The coroutine one silently drops the
`is_closure` branch — so closures-as-coroutine-args take a different path than
closures-as-fn-args, with no comment.

**Proposed fix:** extract one `lower_param_type_to_llvm(Type *t, ctx, module) ->
LLVMTypeRef` (a flat switch on `t`'s shape) used by both. `codegen_fn_type`
becomes a thin loop over params; `codegen_coro_fn_type` becomes the same loop
with the extra leading param. Eliminates the divergence and the 70-line
duplication.

---

## 3. Generic-function specialization: three near-identical dispatch paths

"Resolve the compile-time type of a function" lives in three places that each
rebuild a `JITLangCtx`, run `unify`+`solve_constraints`, build an env from the
subst, and walk the fn-type binding generic args into the env:

- `compile_specific_fn` (function.c:452-501) — for plain generic functions.
- `coro_create_from_generic` (coroutines.c:198-288) — for coroutine constructors.
- `build_generic_closure_value` (closures.c:307-379) — for generic closures.

All three contain the same block (function.c:476-496, coroutines.c:249-275,
closures.c:317-320):
```c
compilation_ctx.type_subst = create_subst_for_generic_fn(generic, specific);
compilation_ctx.env = create_env_from_subst(env, subst);
while (specific_type->kind == T_FN) {
  Type *from = specific_type->data.T_FN.from;
  if (is_generic(from)) {
    Type *r = specialize_type_for_codegen(from, &compilation_ctx);
    if (r) compilation_ctx.env = codegen_bind_in_env(compilation_ctx.env, from, r);
  }
  specific_type = specific_type->data.T_FN.to;
}
```
plus the specific-fn caching via `specific_fns_lookup`/`specific_fns_extend`
(function.c:503-541, coroutines.c:236-284).

A *fourth* related path is `resolve_sym_type` (application.c:108-118) which
does the unify+solve but *not* the env walk, and is used by
`call_generic_function`. So the "unify expected vs symbol type, build subst,
specialize" concept appears 4× with inconsistent completeness.

**Why it's hard to follow:** to answer "what happens when I call a generic
fn" you must read `call_generic_function` -> `resolve_sym_type` ->
`get_specific_callable` -> `compile_specific_fn`, but the coroutine path is
`coro_create_with_reset_closure` -> `coro_create_from_generic` (which re-implements
the cache+compile rather than calling `get_specific_callable`). The closure
path is `call_generic_closure_sym` -> `build_generic_closure_value` (a third
cache+compile). Three caching sites for "specific fns" that could drift.

**Proposed fix:** one function
```c
JITLangCtx make_specialization_ctx(JITSymbol *sym, Type *specific_type, JITLangCtx *parent);
```
that does subst+env+arg-bind once, and one cache-aware
```c
LLVMValueRef get_or_compile_specific(JITSymbol *sym, Type *specific_type,
                                      LLVMValueRef (*compile)(Ast*, JITLangCtx*, ...));
```
where `compile_specific_fn`, `coro_create_from_generic`, and
`build_generic_closure_value` each supply only their compile callback. The
generic-arg env walk currently *missing* from `resolve_sym_type` becomes
automatic.

---

## 4. `bind_fn_param` / `bind_fn_param_with_storage`: duplicated closure-vs-fn arms

`function.c:182-225` (`bind_fn_param`) and `function.c:227-269`
(`bind_fn_param_with_storage`) share identical `is_closure`/`T_FN` arms —
both do the same `new_symbol(STYPE_FUNCTION, ...)` + `ht_set_hash` for the
closure and plain-fn cases. They differ only in the `else` branch (one calls
`codegen_pattern_binding`, the other a `switch` on `AST_IDENTIFIER` calling
`bind_local_value_with_storage`).

```c
// identical in both (function.c:197-219 vs 236-258):
if (param_type->kind == T_FN && is_closure(param_type)) {
  ... new_symbol(STYPE_FUNCTION, param_type, param_val, rec_type); ht_set_hash(...)
} else if (param_type->kind == T_FN) {
  ... new_symbol(STYPE_FUNCTION, param_type, param_val, llvm_type); ht_set_hash(...)
}
```

**Why it's hard to follow:** two functions with one-line name differences and
near-identical bodies; a reader must diff them line-by-line to find the real
difference (the `else` branch only handles `AST_IDENTIFIER` in the
`_with_storage` variant — other param AST shapes silently do nothing there,
which is a latent gap vs the non-storage version that calls
`codegen_pattern_binding`).

**Proposed fix:** factor the closure/fn binding into a `bind_callable_param`
helper; each variant keeps only its distinct value branch.

---

## 5. The coroutine "drain inner" loop idiom: emitted inline 4× + one helper that isn't reused

The pattern "resume inner, check done, read promise, yield, loop" appears
inline in:

- `codegen_yield` (coroutines.c:675-796) — the `yield_from` branch, ~120 lines
  of inline CFG,
- `CorLoopHandler` (coroutine_extensions.c:139-207) — inline drain loop,
- `CorMapHandler` (coroutine_extensions.c:501-589) — same loop + a map call,
- `CorFilterHandler` (coroutine_extensions.c:789-887) — same loop + predicate,
- `CorOfCorListHandler` (coroutine_extensions.c:1584-1672) — nested variant,
- `CorTakeHandler` (coroutine_extensions.c:1165-1273) — same loop + counter.

Meanwhile `coro_emit_yield_from_loop` (coroutines.c:1501-1606) **already
encapsulates exactly this** — and is documented as "For use in cor_loop,
cor_map, user yield-from, etc." — but **none of the handlers use it.** They
each re-emit `loop_check_bb`/`loop_body_bb`/`get_value_bb`/`loop_resume_bb`/
`suspend_return_bb` + the same `coro.done`/`coro.resume`/`coro.promise`/
`coro.save`/`coro.suspend` calls by hand.

**Why it's hard to follow:** the helper exists and is well-named, but the code
doesn't use it, so a reader looking for "how does cor_map drain its inner
coroutine" reads 90 lines of raw LLVM in `CorMapHandler` instead of a 3-line
call. The inline versions also each have subtle differences
(`CorLoopHandler` resets the inner via memcpy and loops forever;
`CorMapHandler`/`CorFilterHandler` finish on exhaustion) — differences that a
helper with a "on_exhaustion" callback would make explicit rather than buried.

**Proposed fix:** drive the handlers through `coro_emit_yield_from_loop` (or a
slightly generalized variant taking an "on value" transform callback + an
"on exhaustion" callback). `CorMapHandler`'s loop body collapses to:
```c
coro_emit_yield_from_loop(ctx, module, builder, handle, inner,
    promise_alloca, llvm_input_type, cleanup_bb, suspend_bb, "map",
    /*transform=*/ map_fn, /*exhaustion=*/ FINAL_SUSPEND);
```
`CorFilterHandler` supplies a predicate-skip transform; `CorLoopHandler`
supplies a reset+loop exhaustion callback. This is the single biggest
readability win in the file.

---

## 6. `coro_create_with_reset_closure`: void/non-void duplication

`coroutines.c:445-472` has two branches (`!is_void_arg` / `else`) that are
nearly identical except the arg-loading loop:

```c
if (!is_void_arg) {
  for (i...) { ...load arg from struct, push to inner_args... }
  inner_handle = LLVMBuildCall2(..., inner_args, 1+len, ...);
  PROMISE_SET_RESET_FN(...); PROMISE_SET_ARGS_PTR(...);
  LLVMBuildRet(builder, inner_handle);
} else {
  inner_handle = LLVMBuildCall2(..., (LLVMValueRef[]){frame_size_out}, 1, ...);
  PROMISE_SET_RESET_FN(...); PROMISE_SET_ARGS_PTR(...);   // <-- duplicated
  LLVMBuildRet(builder, inner_handle);                    // <-- duplicated
}
```
The `PROMISE_SET_*` + `Ret` pair is repeated verbatim. The same shape repeats
again at coroutines.c:502-508 (the caller side, ternary over `is_void_arg`).
A large commented-out `coro_create_reset_fn_from_handle` (coroutines.c:329-380)
sits above it as dead code.

**Why it's hard to follow:** the meaningful difference (load N args vs 0 args)
is buried inside two 15-line blocks that are 80% identical, with the
actually-shared tail duplicated. The dead commented block above adds noise.

**Proposed fix:** emit the arg loop conditionally, then a single shared tail
(`PROMISE_SET_*` + `Ret`). Delete the commented-out function.

---

## 7. `codegen_application` dispatch: flat but with an unreachable tail and an early `is_closure` double-check

`application.c:226-292` is a reasonable flat `if`-chain dispatch on `sym->type`
— good. Two readability nits:

- The chain ends with `return NULL;` (application.c:291) with no diagnostic, so
  a fall-through silently yields NULL. Every other path prints an error first.
  Add an "unhandled symbol type" `fprintf` there.
- `call_closure_sym` (closures.c:400-411) re-checks
  `sym->type == STYPE_GENERIC_FUNCTION` and re-dispatches to
  `call_generic_closure_sym`, but `codegen_application` *already* special-cased
  `is_closure_symbol(sym)` (application.c:259) before reaching the
  `STYPE_GENERIC_FUNCTION` path (application.c:273). So a generic *closure*
  symbol takes the `is_closure_symbol` branch, and the `STYPE_GENERIC_FUNCTION`
  check inside `call_closure_sym` is dead for the application path (it's only
  reachable via `call_generic_closure_sym` recursion). The double gate is
  confusing.

**Why it's hard to follow:** the dispatch reads as "closure symbols go to
`call_closure_sym`, generic-fn symbols go to `call_generic_function`" — but
`call_closure_sym` *also* handles the generic case, so the real routing is
"closure symbols (generic or not) -> call_closure_sym -> maybe
call_generic_closure_sym; generic non-closure -> call_generic_function". That
nuance is invisible at the dispatch site.

**Proposed fix:** make `call_closure_sym`'s generic-vs-direct split explicit at
the application dispatch (e.g. a `call_generic_closure_sym` case), or document
that `is_closure_symbol` is checked first.

---

## 8. `compile_specific_fn`: generic-arg env walk uses raw type-mutation

`function.c:486-496` mutates the *specific_type* cursor in place while binding
generic args into the env:
```c
while (specific_type->kind == T_FN) {
  Type *f = specific_type->data.T_FN.from;
  if (is_generic(f)) { ... codegen_bind_in_env(env, f, r); }
  specific_type = specific_type->data.T_FN.to;
}
```
`specific_type` was deep-copied earlier? No — it's the `expected_fn_type`
passed in from `get_specific_callable` (function.c:534), which is the *caller's*
type (e.g. `ast->type` for an identifier load at symbols.c:309). The walk only
*reads* `specific_type` (no writes), so it's safe, but the variable name
suggests ownership and the deep-copy question is non-obvious. Compare with
`coro_create_from_generic` (coroutines.c:257) which *does* deep-copy first —
inconsistent treatment of the same pattern (#3 again).

**Proposed fix:** consistent deep-copy (or consistent no-copy with a comment)
across the three specialization sites.

---

## 9. `AST_TYPE_DECL` case in codegen: dead/unreachable `else if` branches

`codegen.c:306-346` has this structure:
```c
if (is_generic(t)) { ... }                      // 306
else if (is_generic(t) && t->kind == T_CONS) { ... }   // 316 -- UNREACHABLE
else if (!is_generic(t) && t->kind == T_CONS) { ... }   // 326
else if (!is_sum_type(t) && t->kind != T_FN) { ... }    // 337
```
The `else if (is_generic(t) && ...)` at 316 can never run (the `if (is_generic(t))`
at 306 already caught all generic types). The three `else if` bodies (316, 326,
337) are also byte-identical:
```c
JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, t, NULL, NULL);
sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = codegen_cons_type_constructor;
ht *stack = (ctx->frame->table);
ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);
```

**Why it's hard to follow:** a reader sees 4 branches and tries to understand
when each fires; in reality only branch 1 (generic) and branch 3 (non-generic
cons) ever fire, and they emit identical code. The dead branch 2 looks
intentional but is provably unreachable.

**Proposed fix:** collapse to `if (is_generic(t) || t->kind == T_CONS) { ... }
else if (!is_sum_type(t) && t->kind != T_FN) { ... }`.

---

## 10. Leftover debug output (live, not commented)

Live `printf`/debug calls left in production paths:

- `builtin_functions.c:698` — `printf("\n");` inside `list_eq`, fires on every
  list-element equality comparison (perf + noise).
- `symbols.c:414-416` — `printf("create closure symbol\n"); print_ast(expr);
  print_type(expr->type);` in `create_closure_symbol`, prints to stdout on
  every closure-symbol creation.
- `jit.c:533` and `jit.c:703` — `printf("\n");` (need context check, but live).

Commented-out debug blocks that should be removed (non-exhaustive):
- `function.c:308-312`, `function.c:340-355`, `function.c:481-484` (print_type
  spam), `closures.c:44-48`, `closures.c:71`, `types.c:46-48,102-112`,
  `codegen.c:229-256` (RECORD ACCESS prints), `coroutines.c:203-234` (the big
  TODO comment block inside `coro_create_from_generic`).
- `match.c`: 350-357, 413, 731, 745-749, 779, 769 (see match.c review).
- `adt.c`: 476-491 (16-line commented recursive-ref validation), 647-648.

**Proposed fix:** remove the live `printf`s; delete commented-out debug
blocks. (Per CLAUDE.md "Surgical Changes": mention, don't delete pre-existing
dead code unless asked — but these are debug artifacts, not logic.)

---

## 11. `codegen_yield` yield-from branch: 120-line inline CFG duplicates `coro_emit_yield_from_loop`

Already covered as part of #5, but calling out `coroutines.c:675-796`
specifically: the `is_coroutine_type(yield_val_type)` branch of `codegen_yield`
re-emits the entire drain loop by hand (loop_check/body/get_value/resume/exit
blocks + save/suspend/switch) — ~120 lines — while `coro_emit_yield_from_loop`
(coroutines.c:1501) does the same thing as a reusable helper. This is the
*same* duplication as the `Cor*Handler`s, just in the user-facing `yield`
path.

**Proposed fix:** route `codegen_yield`'s yield-from through
`coro_emit_yield_from_loop` (with an "on exhaustion: continue outer" callback,
since user yield-from continues the outer coroutine rather than final-suspending).

---

## 12. match.c: `test_sum_type_pattern` double-ladder + tripled unreachable terminator

(match.c:444-593, from the match.c/adt.c deep-dive.) The function builds a
tag-check phase and a payload-test phase, but emits the same
`if (p->tag == AST_IDENTIFIER) ... else if (p->tag == AST_APPLICATION) ...`
ladder **twice** (match.c:505-529 and 538-557), differing only in
`tag_succ`/`test_succ` and `next_tag_group_idx` vs `pidx+1`. The
`if (pidx == num_branches - 1) { ... LLVMPositionBuilderAtEnd; LLVMBuildUnreachable; }`
terminator appears 3× (533-536, 577-580, 588-591).

**Why it's hard to follow:** reading this requires holding "which phase am I
in (tag vs test), am I the last branch, and which block am I positioning"
across ~90 lines. `next_tag_group_idx` is called inline as an array index
(match.c:507, 520), hiding that it scans forward to a different constructor.

**Proposed fix:** one helper `emit_branch_fail_target(p, pidx, phase, blocks)`
returning the fail block + a `bool is_last`; call it in both phases. Hoist
`next_tag_group_idx` out of the subscript into a named local.

---

## 13. match.c: `codegen_match` duplicates the branch tail/merge logic

(match.c:595-889.) The `is_direct_bool_skip_match` fast path (655-725) copies
the main loop's tail/merge emission (828-849): same
`LLVMGetBasicBlockTerminator` checks, same tail-call + `build_ret`, same
`LLVMAddIncoming(result_phi, ... undef ...)` handling. The wildcard unwrap
(`AST_MATCH_GUARD_CLAUSE` -> `test_expr`, then `ast_is_placeholder_id`) is
triplicated (match.c:43-45, 152-162, 208-221).

**Proposed fix:** unify the per-branch tail/merge into one helper; extract
`unwrap_pattern_and_is_wildcard(p)`.

---

## 14. adt.c: triplicated "find variant by name" + `cast_union` arm duplication

(From the adt.c deep-dive.)

- The "scan variants to find index by name" idiom appears 3×: adt.c:30-37
  (`codegen_simple_enum_member`), 61-68 (`codegen_adt_member`), 80-83
  (`codegen_adt_member_with_args`). The third is an *unbounded* `while` with
  no loop guard — if the name isn't a variant it reads off the end of `args[]`.

- `cast_union` (adt.c:509-583) has 3 arms (`scalar`/`T_FN`/`struct`) that are
  the same "alloca -> store -> bitcast ptr -> load2" pattern differing only in
  the element type. `codegen_adt_member_with_args` then *re-inlines* the struct
  arm (adt.c:111-121, 131-141) instead of calling `cast_union`.

**Proposed fix:** `find_variant_index(enum_type, name)` (bounded) helper used
by all three; parameterize `cast_union` by element type and reuse it in the
constructor.

---

## 15. Dead code flagged (mention only, per surgical-changes guideline)

- `adt.c:157-182` `get_largest_type` — no callers (sibling
  `get_largest_type_size` is used).
- `adt.c:436-452` `find_recursive_type_container` — only reachable from the
  commented block at adt.c:476-491; effectively dead.
- `adt.c:294` `extract_tag`: `LLVMStructType((LLVMTypeRef[]){TAG_TYPE},1,0)` is
  constructed fresh each call; anonymous structs aren't interned, so the
  equality is always false — that branch is unreachable as written (looks
  intentional, isn't).
- `function.c:543-545` `codegen_fn_compose` — empty function body (just `{}`).
- `coroutines.c:329-380` commented-out `coro_create_reset_fn_from_handle`.

---

## Priority summary (by readability impact)

```tsv
#	Area	Impact	Effort
1	Coroutine wrapper prologue/epilogue boilerplate (#1, #5, #11)	Very high	Medium
5	Reuse coro_emit_yield_from_loop in handlers + codegen_yield	Very high	Medium
2	Unify codegen_fn_type / codegen_coro_fn_type (#2)	High	Low
3	Unify generic-fn specialization dispatch (#3, #8)	High	Medium
4	bind_fn_param vs _with_storage (#4)	Medium	Low
9	AST_TYPE_DECL dead branches (#9)	Medium	Low
12-14	match.c/adt.c ladders + triplication (#12,13,14)	Medium	Medium
6	coro_create_with_reset_closure void/non-void (#6)	Low	Low
7	codegen_application dispatch clarity (#7)	Low	Low
10	Remove live debug printf + commented debug (#10)	Low	Low
15	Dead code (#15)	Low	Low
```

No files were modified. This document is the deliverable.
