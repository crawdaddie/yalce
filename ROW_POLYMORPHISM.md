# Row Polymorphism Design Sketch

This document captures a pragmatic row-polymorphism design that fits the
current Yalce type system (tuples with optional field names) and supports
flattening-style operations such as `cor_zip`.

## Goals

- Represent tuples/records as rows with optional open tails.
- Allow type-level record extension/concatenation (formal basis for tuple
  flattening).
- Preserve current LLVM tuple codegen (rows are erased at runtime).
- Keep changes localized to type inference + unification where possible.

## Core Model

A row is a set of labeled fields with an optional row tail variable:

- Closed row: `{ a: A, b: B }`
- Open row: `{ a: A, b: B | ρ }`

Treat named tuples as records with labels. For unnamed tuples, use numeric
labels (`_0`, `_1`, ...), or keep a parallel "positional labels" scheme.

## Type Representation (Suggested)

Add a row type to the type system:

- `T_ROW`: fields + optional tail
  - `fields`: list of `{label, Type*}`
  - `tail`: optional type variable representing the rest of the row

Represent records/tuples as `T_CONS` with name `"Record"` (or reuse
`"Tuple"`) but store a row internally, or add `T_RECORD` that references a row.

Existing named tuple layout:

- Today: `Type{T_CONS, name=Tuple, args=[...], names=[...]}`
- Proposed: `Record(Row)` where row holds `(name -> type)` mapping

## Unification (Row-Polymorphic)

Unifying records requires:

- Matching labels unify their field types.
- Labels that appear only on one side are pushed into the other side's tail.
- If both sides are closed (no tail), extra labels cause unification failure.

This enables partial/extendable records:

- `{ a: A | ρ }` unifies with `{ a: A, b: B }` by solving `ρ = { b: B }`.

## Row Extension / Concatenation

Define a type-level operation (conceptually):

- `extend({ fields | ρ }, k: T) = { fields ∪ {k:T} | ρ }`

Used for tuple flattening and `cor_zip`.

## cor_zip with Rows

Desired behavior:

- `Coroutine<T> -> Coroutine<U> -> Coroutine<(T, U)>`
- If `T` is already a tuple (record), flatten into `(a, b, c)`

With rows:

- If left yields `Record(Row)` and right yields `U`, output is
  `Record(Row extended with new label for U)`
- If left yields non-record `T`, treat as `Record({_0: T})` and extend

This gives formal flattening without special-casing tuples at runtime.

## Runtime / LLVM Codegen

Rows erase at runtime. After type inference resolves the concrete fields,
codegen emits a fixed LLVM struct identical to current tuple handling.

Implementation detail:

- For unnamed tuples, keep an ordered label scheme so the final field order
  is deterministic (e.g. `_0`, `_1`, ...). This avoids ambiguity in LLVM
  struct layout.

## Minimal Migration Path

1. Add `T_ROW` and row tail variables to `Type`.
2. Adapt tuple/record types to be backed by a row (or add `T_RECORD`).
3. Update unification for record/tuple types to support open rows.
4. Special-case `cor_zip` in inference using row extension (or, later,
   make it generic).
5. Keep LLVM tuple/record codegen unchanged once final field order is fixed.

## Notes / Tradeoffs

- This is not vanilla Hindley–Milner; it's HM + row polymorphism.
- Requires row unification and row variables in inference.
- Provides a principled basis for flattening and record extension.


## Formal Inference Rules (Sketch)

Below is a compact, HM-style presentation with row polymorphism. Let rows be
written as `r` and row variables as `ρ`. A record type is `Record(r)`.

### Row syntax

```
row r ::= · | r, (l : τ) | ρ
```

### Unification (rows)

Row unification is defined by label matching plus tail propagation.
We write `r1 ≈ r2` to mean "unify rows" and yield substitutions.

**Row-Empty**
```
· ≈ ·
```

**Row-Extend (label present)**
```
(l:τ, r1) ≈ (l:τ', r2)    ⇒    τ ≈ τ'  and  r1 ≈ r2
```

**Row-Swap (label missing)**
```
(l:τ, r1) ≈ (r2)    where l ∉ labels(r2)
⇒ r1 ≈ (r2, l:τ)
```

**Row-Tail**
```
(l:τ, r1) ≈ ρ    ⇒    ρ := (l:τ, r1)    (if ρ not in r1)
```

These are the standard row-polymorphic unification steps; implement with
"row flattening" and occurs-check on ρ.

### Record types

```
Γ ⊢ e : τ
———————————————
Γ ⊢ e : Record(r)    (where r corresponds to τ)
```

In practice, `Record(r)` is the underlying tuple/struct type.

### cor_zip

Let `Coroutine(τ)` denote coroutine instance type.

**Zip (general):**
```
Γ ⊢ c1 : Coroutine(Record(r))
Γ ⊢ c2 : Coroutine(τ)
———————————————————————————————————————————————
Γ ⊢ cor_zip c1 c2 : Coroutine(Record(r ⊕ {k:τ}))
```

Here `⊕` is row extension with fresh label `k`.

**Zip (non-record left):**
```
Γ ⊢ c1 : Coroutine(τ1)   and τ1 not a Record
Γ ⊢ c2 : Coroutine(τ2)
———————————————————————————————————————————————
Γ ⊢ cor_zip c1 c2 : Coroutine(Record({_0:τ1, _1:τ2}))
```

**Zip (record left with positional labels):**
If using numeric labels, choose `k` as next index (`_n+1`).

### Tuple/Record Construction

If you keep tuple syntax but want row-polymorphic typing:

```
Γ ⊢ e1 : τ1 ... Γ ⊢ en : τn
———————————————————————————————
Γ ⊢ (e1, ..., en) : Record({_0:τ1, ..., _n-1:τn})
```

Named tuple:

```
Γ ⊢ e1 : τ1 ... Γ ⊢ en : τn
———————————————————————————————
Γ ⊢ (a=e1, ..., n=en) : Record({a:τ1, ..., n:τn})
```

### Field Access

```
Γ ⊢ e : Record(r)
(l:τ) ∈ r
———————————————
Γ ⊢ e.l : τ
```

### Record Extension (if exposed)

```
Γ ⊢ e : Record(r)
Γ ⊢ v : τ
l ∉ labels(r)
———————————————————————————————
Γ ⊢ extend(e, l, v) : Record(r ⊕ {l:τ})
```

