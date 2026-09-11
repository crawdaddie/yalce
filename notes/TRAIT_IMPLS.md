# Trait Impls & Declarations

Design notes for trait registration (already fixed) and trait *declarations*
(not yet implemented) in the new predicate-based type system. Grounded in the
current code; nothing here is implemented unless noted.

Scope: `lang/types/inference.c` (`infer_expr`), `lang/types/type.c`
(`TypeClass` / `typeclasses_extend`), `lang/types/typeclass_resolve.c`,
`lang/types/builtins.c` (builtin typeclasses), `lang/backend_llvm/codegen.c`
(`AST_TRAIT_IMPL` dispatch).

---

## 1. Current state

### 1.1 `TypeClass` representation

A trait is a `TypeClass` (type.h:17):

```c
typedef struct TypeClass {
  const char *name;
  double rank;          // disambiguation for COMPARABLE predicates
  Type *module;         // interface module of method schemes (see §3)
  TypeList *params;     // Self / multi-param type vars
  struct TypeClass *next;
} TypeClass;
```

Key facts:
- `module` is set by the recent `AST_TRAIT_IMPL` fix (see §2) to the inferred
  impl-module type, but is **never read** for signature lookup today.
- `params` is never populated by any current code path.
- A type "implements" a trait by having a `TypeClass` node in its
  `t->implements` linked list, attached via `typeclasses_extend` (type.c:842).

### 1.2 How a type comes to implement a trait

`typeclasses_extend(t, tc)` (type.c:842) appends `tc` to `t->implements` if
not already present. This is the *only* mechanism. It's called from:

- **builtins:** `typeclasses_extend(&t_int, &tc_int_arith)` etc.
  (builtins.c:830-835), where `tc_int_arith` is a static `TypeClass` with
  `name = "Arithmetic"`.
- **the recent fix:** the new `case AST_TRAIT_IMPL` in `infer_expr`
  (inference.c) does `typeclasses_extend(target, tc)` with a freshly
  `t_alloc`'d `TypeClass` carrying the trait name + the inferred impl module.

`get_typeclass_by_name` (type.c:~718), `get_typeclass_instance` (type.c:~730),
`type_implements` (type.c:745), and `resolve_predicates` (inference.c:1204)
all walk `t->implements` comparing `tc->name` by string. So trait identity is
**by name**.

### 1.3 How dispatch works today (the hardcoded path)

Trait method dispatch is **per-trait and hand-written in the backend**, not
generic. Example: arithmetic binops via `ARITHMETIC_BINOP`
(builtin_functions.c:142):

```c
#define ARITHMETIC_BINOP(_name, _flop, _iop) ({
  Type *ret = fn_return_type(fn_type);
  switch (ret->kind) {
  case T_INT: case T_UINT64: { ... LLVMBuildBinOp(builder, _iop, ...) ... }
  case T_NUM: { ... LLVMBuildBinOp(builder, _flop, ...) ... }
  default: {
    if (ret->alias) {
      JITSymbol *sym = get_typeclass_method(ret->alias, _name, ctx);
      // ... call sym ...
    }
  }
  }
})
```

`get_typeclass_method` (builtin_functions.c:90) resolves by **string
convention**, not by the trait's `module`:

```c
JITSymbol *get_typeclass_method(char *type_name, char *op, JITLangCtx *ctx) {
  char chars[total_chars];
  sprintf(chars, "%s.%s", type_name, op);   // e.g. "Synth.+"
  return find_in_ctx(chars, total_chars, ctx);
}
```

So methods are stored as symbols named `"<Type>.<op>"` and looked up by that
convention. `Constructor` and `Arithmetic` each have their own backend handler
(`create_constructor_module`, `create_arithmetic_typeclass_methods`,
codegen.c:371). There is **no generic "ask trait T for method m on type X"**
path. The four builtin trait names are special-cased in codegen.

### 1.4 The predicate side (already generic)

The *constraint* machinery is trait-name-generic:
- `resolve_predicates` (inference.c:1204) iterates predicates and, for
  `PRED_TRAIT`, calls `get_typeclass_instance(t, p->trait->name, params)` —
  name-based, works for any trait.
- `create_tc_resolve` (type.c:875) builds a `T_TYPECLASS_RESOLVE` carrying the
  trait name; `resolve_tc_rank` (typeclass_resolve.c:82) resolves it by
  `get_typeclass_rank(arg, name)`.

So inference *checks* trait obligations generically; only *dispatch* is
hardcoded. A declared trait that produces a `TypeClass` with the right name is
immediately usable by the predicate resolver.

### 1.5 What was broken and is now fixed

`AST_TRAIT_IMPL` (`let X : Trait = module ...`, parser.y:283) had no case in
`infer_expr` — it fell through to `default: break;`, returning `NULL`, failing
the enclosing module inference with "failed to infer module ... as T_MODULE".

The fix (inference.c, new `case AST_TRAIT_IMPL`): infer the impl module body,
look up the named target type via `env_lookup`/`lookup_builtin_type`, build a
`TypeClass{name, module=impl_type}`, `typeclasses_extend(target, tc)`, set
`ast->type = impl_type`. This makes `open engine/bindings/Synth;` typecheck.

### 1.6 The gap a trait *declaration* fills

There is currently **no trait registry** and no way to declare a new trait. The
four builtin traits are static `TypeClass` structs (`_GenericArithmetic` etc.,
builtins.c:129-139) exposed as globals. `AST_TRAIT_IMPL`'s name lookup goes
through `env_lookup`/`lookup_builtin_type` — which works for builtin traits
only because they happen to be types in the builtin env, and works for a user
trait **only if** the trait name resolves to something. A user-defined trait
has nothing to resolve to.

So: trait registration exists (§2); trait *declaration* (naming a new trait +
its method interface + registering it) does not. The rest of this doc designs
the declaration.

---

## 2. Trait registration (implemented)

### 2.1 The `AST_TRAIT_IMPL` node

```
let X : Trait = module () -> ... ;;
```
parses (parser.y:283) to `AST_TRAIT_IMPL` (parse.h:285):

```c
struct AST_TRAIT_IMPL {
  Ast *impl;            // the module lambda
  ObjString type;       // the type name being implemented ("Synth")
  ObjString trait_name; // the trait name ("Constructor")
} AST_TRAIT_IMPL;
```

### 2.2 The registration case (inference.c)

```c
case AST_TRAIT_IMPL: {
  Ast *impl = ast->data.AST_TRAIT_IMPL.impl;
  Type *impl_type = infer_expr(impl, ctx);
  if (!impl_type) return NULL;

  const char *trait_name = ast->data.AST_TRAIT_IMPL.trait_name.chars;
  const char *type_name  = ast->data.AST_TRAIT_IMPL.type.chars;

  Type *target = env_lookup(ctx->env, type_name);
  if (!target) target = lookup_builtin_type(type_name);
  if (!target) { type_error(...); return NULL; }

  TypeClass *tc = t_alloc(sizeof(TypeClass));
  *tc = (TypeClass){.name = trait_name, .module = impl_type};
  typeclasses_extend(target, tc);

  ast->type = impl_type;
  type = impl_type;
  break;
}
```

### 2.3 What it does and doesn't do

- **Does:** typechecks the impl module body (ordinary lets), attaches a
  `TypeClass` to the target type's `implements` list, returns a non-NULL module
  type so the enclosing module infers as `T_MODULE`.
- **Does not:** check the impl against the trait's interface (no interface
  exists yet for user traits), validate method signatures, handle `Self`.
- **Limitation:** trait name lookup is via the type env, so only builtin traits
  (which are types) resolve. A user trait needs the registry (§3).

---

## 3. What a trait declaration must mean

A trait is a **set of method signatures that a type promises to provide, plus a
way to look up those methods given a concrete type.** The concept is partially
built but split across three things that aren't unified:

### 3.1 The trait's interface (its method signatures)

`TypeClass.module` is where this lives: a `T_MODULE` whose entries are the
method schemes. Declaring a trait = producing that interface `T_MODULE` and
binding it under the trait name. Today `module` is set to the *impl* module
(§2), never to an *interface* module, and never read. A declaration would set
it to the interface and read it at the impl site.

### 3.2 The trait's instances (which types implement it)

`t->implements` (linked list of `TypeClass`), attached by
`typeclasses_extend`. Each instance's `module` is the impl module, `name`
matches the trait. Already working (§2). `get_typeclass_by_name` /
`type_implements` / `resolve_predicates` all walk this list by name.

### 3.3 The dispatch (given concrete type + trait method, find the function)

Today: hardcoded per-trait in the backend (`ARITHMETIC_BINOP` switch +
`"<type>.<op>"` string convention, §1.3). A declared trait needs a generic
path: `get_typeclass_by_name(t, trait_name)` -> `tc->module` -> member lookup
by method name. The member lookup already exists (`AST_RECORD_ACCESS` over
`T_MODULE`, inference.c:978). The missing glue is "given type T and trait
name, fetch the instance module."

### 3.4 The four things a declaration must establish

| # | what | mechanism |
|---|------|----------|
| (a) | Bind trait name -> `TypeClass` carrying the interface | a trait registry (ht) |
| (b) | Make method signatures available for impl-checking | `TypeClass.module` = interface `T_MODULE` |
| (c) | Handle `Self` / the implementing type | `TypeClass.params`, substituted at impl site |
| (d) | Enable generic dispatch | `get_typeclass_by_name` -> `tc->module` -> member lookup |

---

## 4. The minimal viable declaration

In order of necessity.

### 4.1 A trait registry

A `ht` mapping trait name -> `TypeClass*`. Builtins seed it with
`GenericArithmetic`/`GenericOrd`/`GenericEq`/`GenericFrom`; a declaration adds
to it. `AST_TRAIT_IMPL`'s name lookup (§2.2) should consult this registry
instead of `env_lookup`/`lookup_builtin_type`, because a user trait is not a
type and won't be in the type env.

Without a registry, `let X : MyTrait = module ...` has nothing to attach to.

### 4.2 An `AST_TRAIT_DECL` node

Holds the trait name and an interface module (method signatures as a
`T_MODULE`). Inference produces a `TypeClass` with `name` + `module` (the
inferred `T_MODULE`) + `params` (the `Self` variable(s)) and registers it in
the registry (§4.1).

The trait's *type* (what `let X : Trait = ...` refers to) is the `TypeClass`
itself, or a thin wrapper — impl resolution compares names, so the `TypeClass`
is sufficient.

### 4.3 `Self` substitution at the impl site

When `AST_TRAIT_IMPL` runs (§2), instead of just attaching the impl module, it
should also check each method: for each `(name, scheme)` in the trait's
interface `module`, find the impl's method of the same name, unify the impl
method's type with `scheme[Self := implementing_type]`. This makes
`let Synth : MyTrait = module () -> let foo = fn x -> ... ;;` *typecheck*
rather than register blindly.

### 4.4 Generic method dispatch (follow-on)

Replace the `"<type>.<op>"` string convention with `tc->module` member lookup,
so any trait's methods are callable as `record_access` on the resolved
typeclass instance. `AST_RECORD_ACCESS` over `T_MODULE` (inference.c:978)
already does member lookup; the only glue is "given type T and trait name, get
the instance module."

---

## 5. What a declaration does *not* need (to stay minimal)

- **No new `T_TRAIT` type kind.** `TypeClass` is already the representation; a
  declared trait is just a `TypeClass` in a registry rather than a static
  global.
- **No coherence / overlap checking.** Rust requires orphan rules +
  uniqueness; defer that and allow multiple impls (first-wins, since
  `get_typeclass_by_name` returns the first match). `typeclasses_extend` guards
  against duplicates via `has_typeclass_instance`, but that compares params
  too, so distinct impls are allowed.
- **No associated types / higher-kinded types.** `TypeClass.params` is a flat
  `TypeList`; `Self` substitution is first-order. Covers
  `Arithmetic`/`Ord`/`Eq`/`From`/`Constructor` and user single/multi-param
  traits.
- **No backend change first.** Inference can register and check traits; the
  hardcoded `ARITHMETIC_BINOP`/`create_constructor_module` dispatch keeps
  working for builtins. Generic dispatch (§4.4) is a follow-on that makes
  *user* traits callable; until then a user trait can be declared and impls
  checked, just not dispatched — still useful (documents the interface, verifies
  conformance).

---

## 6. The key design decision: `Self` explicit or implicit

The one choice that shapes the rest.

### 6.1 Implicit `Self` (Rust-style)

`trait Add with fn add : Self -> Self -> Self`. The interface module's entries
use `Self`, which the declaration introduces as a type var bound to `params`.
Impl site substitutes the concrete type for `Self`. More expressive (can
express `Self -> Self` constraints); requires threading `Self` through the
interface schemes.

### 6.2 Explicit, no `Self` (what builtins effectively do today)

The interface just has method types with a placeholder for the implementing
type; the impl is checked by unifying the impl's method type directly. This is
what `Arithmetic` does: `t_int` implements `Arithmetic` by having
`add`/`sub`/`mul`/`div`/`mod` as plain functions, no `Self` anywhere. Less
expressive but needs no `Self` machinery.

### 6.3 Recommendation

The builtins are already the explicit/no-`Self` style. The path of least
resistance: a declaration produces an interface `T_MODULE` of plain method types
(no `Self`), impl-checking unifies impl methods against those types after
substituting the implementing type for the first parameter. This generalizes
what builtins do without introducing `Self`. Add `Self` later if
`Self -> Self`-style constraints are needed — it's additive.

---

## 7. Putting it together: a user trait, end to end

Given the goal syntax (illustrative):

```
trait MyTrait with
  fn foo : Int -> Int;
end

let Synth : MyTrait = module () ->
  let foo = fn x -> x + 1 ;;
;
```

What happens:

1. **Declaration** (`trait MyTrait with ...`): infer the interface `T_MODULE`
   (`{foo : Int -> Int}`), build `TypeClass{name="MyTrait", module=interface,
   params=[Self_or_none]}`, insert into the trait registry under "MyTrait".
2. **Impl** (`let Synth : MyTrait = module ...`): the `AST_TRAIT_IMPL` case
   looks up "MyTrait" in the **registry** (not the type env), infers the impl
   module, checks each impl method (`foo : Int -> Int` inferred) against the
   interface's `foo` (unify, substituting `Synth` for the receiver position if
   `Self` is used), then `typeclasses_extend(synth_type, tc)`.
3. **Predicate resolution**: a `PRED_TRAIT(MyTrait, T)` predicate resolves via
   `get_typeclass_instance(T, "MyTrait", params)` — already generic, works
   immediately once the `TypeClass` is attached.
4. **Dispatch** (later): `t.foo` or `MyTrait.foo(t)` resolves via
   `get_typeclass_by_name(t, "MyTrait")` -> `tc->module` -> member lookup of
   "foo" — reusing `AST_RECORD_ACCESS` over `T_MODULE`.

Steps 1-3 are the declaration+registration+checking design; step 4 is the
generic dispatch follow-on.

---

## 8. Summary table

| concern | status | mechanism |
|---|---|---|
| trait registration (`let X : Trait = module`) | **implemented** (§2) | `AST_TRAIT_IMPL` case in `infer_expr` |
| trait name lookup for impls | partial (builtins only) | `env_lookup`/`lookup_builtin_type`; needs registry (§4.1) |
| interface storage | field exists, unused | `TypeClass.module` -> set to interface `T_MODULE` by declaration |
| impl checking against interface | not done | unify impl methods vs interface schemes at impl site (§4.3) |
| `Self` handling | not done | `TypeClass.params` + substitution (§6) |
| predicate resolution | **generic already** | `resolve_predicates` + `get_typeclass_instance` (name-based) |
| method dispatch | hardcoded per-builtin | `"<type>.<op>"` convention; generalize to `tc->module` lookup (§4.4) |
| trait registry | does not exist | new `ht` name -> `TypeClass*` (§4.1) |
| `AST_TRAIT_DECL` node | does not exist | new parse rule + `infer_expr` case (§4.2) |

### Implementation order

1. Trait registry (`ht`) + seed with builtins; make `AST_TRAIT_IMPL` consult it
   (replaces `env_lookup`/`lookup_builtin_type` in §2.2).
2. `AST_TRAIT_DECL`: infer interface `T_MODULE`, build `TypeClass`, register.
   No `Self`, no impl-checking yet — just makes a user trait name resolvable
   and attaches an interface.
3. Impl-checking: at the impl site, unify impl methods against the registered
   interface (substitute implementing type for receiver). First-order, no
   `Self`.
4. Generic dispatch: `tc->module` member lookup, replacing the
   `"<type>.<op>"` convention. Makes user traits callable.
5. (Optional) `Self` as a first-class interface type var (§6.1), if
   `Self`-reflexive signatures are needed.

Steps 1-3 make user traits declarable and checkable; step 4 makes them
dispatchable; step 5 is additive expressiveness.
