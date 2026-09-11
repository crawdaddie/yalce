# Type Inference Arena Analysis

This is analysis only. It does not propose applying the change directly yet.

## Short answer

A `DECLARE_ARENA_ALLOCATOR`-style allocator can work for the type-inference
pipeline, but not as one resettable arena for every object currently allocated
through `t_alloc`.

The current type data has at least two different lifetimes:

- Temporary inference data that can die after solving/finalization.
- Semantic type data that must survive inference because codegen, the REPL,
  imported modules, and AST annotations still reference it.

So the practical design is two arenas, not one:

- A long-lived semantic/type arena for final types, persistent `TypeEnv`s,
  builtins, module types, schemes, and AST-attached types.
- A short-lived inference arena for constraints, substitutions, freshening maps,
  temporary predicate copies, and other solver scratch state.

## Current state

`lang/types/inference.h` currently includes `lang/arena_allocator.h` and expands:

```c
DECLARE_ARENA_ALLOCATOR_DEFAULT(t);
```

This is important: the macro declares `static` storage and `static` functions.
Because it is in a header, every `.c` file that includes `inference.h` gets its
own private `t_arena`, `t_alloc`, `t_reset`, and stats helpers.

That means the current `t_alloc` setup is already arena-like, but it is not one
shared type arena. It is many per-translation-unit arenas with the same static
function names.

Consequences:

- Calling `t_reset()` from one `.c` file would reset only that file's private
  arena.
- Allocations from `builtins.c`, `inference.c`, `type.c`,
  `freshen_map.c`, `subst_table.c`, and backend files are not in the same
  arena.
- Current arena stats such as `t_total_used()` are also per translation unit,
  not global type-system stats.
- Moving to a single global allocator later will change lifetime behavior
  because objects that are currently isolated by translation unit would start
  sharing one arena.

## Persistent data currently allocated through `t_alloc`

Several `t_alloc` allocation sites create data that survives the immediate
inference pass.

Persistent examples:

- `TypeEnv` nodes from `env_extend()` / `env_extend_with_preds()` in
  `lang/types/inference.c`.
- Final AST annotations. `finalize_ast_types()` rewrites `ast->type` using
  `apply_subst_to_type()`, and those types are then consumed by escape analysis
  and codegen.
- REPL environment entries. `repl_process_line()` and `repl_loop()` assign
  `ctx->env = ti_ctx.env`, so inferred env entries survive into later REPL
  inputs.
- Imported module types and envs. `init_import()` stores `mod->type` and
  `mod->env` from `infer(mod->ast, &mod_ctx)`.
- `T_MODULE` env copies and `ModuleTypeMeta` created in `inference.c` and
  `freshen_map.c`.
- Builtin env entries, builtin schemes, scheme variables, and builtin
  predicates created in `initialize_builtin_types()`.
- Generic function/module symbol metadata in the LLVM backend stores type envs
  for later specialization.

These cannot live in a per-inference arena if that arena is reset after
`infer()`.

## Temporary data currently allocated through `t_alloc`

Other allocation sites are good candidates for a resettable inference arena:

- `Constraint` nodes from `add_constraint()`.
- `Subst` tables and their backing arrays in `subst_table.c`.
- `FreshenMap` arrays used during instantiation/freshening.
- Temporary `TypeList` values used by free-variable collection.
- Temporary `TypeEnv **entries` arrays used for ordered finalization.
- Predicate copies used while solving/checkpointing, as long as unresolved
  predicates that remain attached to env entries are promoted.
- Fresh type variables and freshly-instantiated types that are guaranteed not
  to be stored in persistent envs or AST annotations without promotion.

The last point is the tricky one. Fresh instantiations often flow through normal
inference and can become part of the returned type, AST annotations, generalized
scheme state, or module metadata. Those results need promotion before the temp
arena is reset.

## Why a naive reset is unsafe

A naive `t_reset()` after `infer()` would create use-after-free bugs because
codegen and later REPL lines use values reachable from:

- `ast->type`
- `ctx->env`
- `YLCModule.type`
- `YLCModule.env`
- `builtin_env_ht`
- `builtin_envs`
- backend generic symbol `type_env` pointers

It would also be incomplete today because `t_alloc` is not one global arena.
Resetting the `inference.c` arena would not clean allocations made from
`builtins.c`, `type.c`, `subst_table.c`, etc.

The current graph also mixes static builtin `Type` objects with arena-allocated
compound types. For example, a persistent function type may point at `&t_bool`
or `&t_num` while also pointing at arena-allocated type variables or `TypeList`
nodes. That is fine, but it means promotion needs to be graph-aware and should
not assume every pointer came from the same arena.

## Recommended design

Introduce explicit type allocation lifetimes.

Suggested names:

```c
void *type_perm_alloc(size_t size);
void type_perm_reset(void);

void *type_tmp_alloc(size_t size);
void type_tmp_reset(void);
```

The semantic/permanent arena should hold:

- Builtin `TypeEnv` entries, schemes, scheme vars, and builtin predicates.
- Finalized `ast->type` graphs.
- `TypeEnv` entries that survive in `ctx->env`.
- Generalized scheme vars and persistent binding predicates.
- Imported module types/envs and module metadata.
- Typeclass instances and `TypeList` parameter lists that are part of live
  types.
- Owned strings such as generated type variable names when the variable
  survives.

The temp/inference arena should hold:

- Constraints.
- Substitution tables and arrays.
- Freshening-map arrays.
- Free-variable scratch lists.
- Ordered finalization arrays.
- Solver-local predicate/type copies.
- Other data that is proven unreachable after finalization or after a failed
  inference pass.

## Promotion requirement

Before resetting the temp arena, final inference results must be copied into
the semantic arena.

Useful helpers:

```c
Type *type_clone_perm(Type *type);
TypeList *typelist_clone_perm(TypeList *list);
Predicate *predicate_clone_perm(Predicate *preds);
TypeEnv *typeenv_clone_perm(TypeEnv *env);
```

The promotion pass should preserve sharing where needed, avoid infinite loops
for recursive/module types, and leave static builtin types alone. A pointer-map
from source pointer to cloned pointer would avoid duplicating shared subgraphs
and would make recursive structures safer.

Good promotion boundaries:

- `infer_final()` after substitution and generalization.
- `finalize_ast_types()` or a follow-up pass that rewrites each `ast->type` to a
  semantic-arena clone.
- `finalize_env_generalization()` / `finalize_env_slice()` for env entries that
  survive beyond the current expression.
- `init_import()` before storing `mod->type` and `mod->env`, unless those are
  already allocated in the semantic arena.

## Migration plan

1. Centralize the allocator first.

   Move type allocator declarations out of `inference.h`. The current macro
   expansion in a header creates private per-file arenas, which makes global
   reset and global diagnostics misleading.

   A small `lang/types/type_alloc.h` / `type_alloc.c` pair would be clearer than
   another header-local macro expansion.

2. Preserve current behavior initially.

   Route existing `t_alloc` to the long-lived semantic arena first. Do not add
   resets yet. This changes the implementation shape without changing
   lifetimes.

3. Add diagnostics.

   Add total allocated/used/block-count stats for the centralized semantic
   arena. If possible, also add an `arena_contains_ptr()` debug helper so later
   assertions can detect temp pointers escaping into persistent structures.

4. Add the temp arena.

   Start moving obviously temporary allocations:

   - `Constraint`
   - `Subst`
   - `FreshenMap`
   - temporary arrays such as ordered `TypeEnv **entries`
   - free-var scratch lists

5. Add promotion and reset.

   Once final AST types and surviving env/module/builtin data are known to be
   semantic-arena allocated, reset the temp arena at the end of `infer()`. Also
   reset it on inference error paths.

6. Audit backend `t_alloc` users separately.

   Several backend files include `inference.h` and call `t_alloc` for codegen
   specialization work. Those allocations may not belong to the inference temp
   arena. They likely need either semantic lifetime, codegen-module lifetime, or
   their own backend scratch arena.

## Guard rails

Before enabling a temp reset by default, add debug checks:

- Assert no temp-arena pointer is reachable from `ast->type` after finalization.
- Assert no temp-arena pointer is reachable from `ctx->env`.
- Assert no temp-arena pointer is reachable from module registry entries.
- Assert no temp-arena pointer is reachable from `builtin_env_ht` or
  `builtin_envs`.
- Run these checks in REPL mode too, because REPL env persistence is exactly
  where a per-inference reset is most likely to expose use-after-free bugs.

## Bottom line

Yes, an arena allocator is a good fit here, but the current `t_alloc` should not
be treated as a single resettable inference arena. The safe path is:

1. Centralize `t_alloc`.
2. Treat current allocations as long-lived until proven otherwise.
3. Add a separate temp inference arena.
4. Promote final AST/env/module data before resetting temp state.

That gives the compiler a clear ownership model while preserving the data that
must survive type inference.
