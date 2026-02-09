# Sequence Pipeline IR — Unified Deforestation for Lists and Coroutines

## Problem

List operations like `map f (map g xs)` produce intermediate lists that are immediately consumed. With the current `map` implementation (`aux f l [] |> rev`), a chain of two maps performs 4 list traversals and 2 intermediate allocations for what could be 1 traversal and 0 intermediates.

The same problem exists for coroutine combinators (`cor_map f (cor_map g cor)` creates intermediate coroutine frames), which is already partially addressed by `fuse_cor_maps` in `coroutine_extensions.c`.

The goal: a unified framework that handles both list and coroutine fusion with a single optimization pass.

## Background: Build/Foldr (Short-cut Deforestation)

Lists have exactly one way to construct (`::` and `[]`) and one universal way to consume (`foldr`):

```
build g   = g (::) []
foldr k z (build g) = g k z    -- fusion rule
```

When `map` is expressed as:
```
map f xs = build (\cons nil -> foldr (\x acc -> cons (f x) acc) nil xs)
```

then `map f (map g xs)` expands, the inner `build` meets the outer `foldr`, and the intermediate list vanishes.

### Coroutine analogy

| Lists | Coroutines |
|-------|-----------|
| `(:)` constructor | `yield` |
| `[]` constructor | `done` / return |
| `foldr` (universal consumer) | `cor_loop` / resume-until-done |
| `build` (abstract producer) | coroutine body (function that calls yield) |

The same fusion rule applies: when a coroutine producer feeds directly into a consumer,
eliminate the intermediate coroutine frame by inlining yields into the consumer.

The existing `fuse_cor_maps` already implements a specialized version of this
— composing transformation functions when adjacent `cor_map` calls are detected.

## Design: Sequence Pipeline IR

Rather than encoding `Sequence` as a type-level relationship (subtyping would break Hindley-Milner), encode it as a **compiler-internal IR** between the typed AST and codegen. Recognize sequence patterns, lift them into a pipeline representation, optimize the pipeline, then lower to concrete code.

### Pipeline structure

A pipeline has three parts:

```
Source  ->  [Op_1 -> Op_2 -> ... -> Op_n]  ->  Sink
```

```
Source = List expr
       | Array expr
       | Coroutine expr
       | Range(from, to)

Op     = Map (T -> U)
       | Filter (T -> Bool)
       | Take Int
       | FlatMap (T -> Seq U)

Sink   = Fold (R -> T -> R) R      -- consume to scalar
       | CollectList               -- materialize as List<T>
       | CollectArray              -- materialize as Array<T>
       | ForEach (T -> Void)       -- side effects only
       | YieldCoroutine            -- produce a Coroutine<T>
```

### Concrete C structure

```c
typedef struct SeqPipeline {
    enum { SRC_LIST, SRC_ARRAY, SRC_COROUTINE, SRC_RANGE } source;
    Ast *source_expr;
    Type *source_el_type;

    int num_ops;
    struct {
        enum { OP_MAP, OP_FILTER, OP_TAKE, OP_FLAT_MAP } kind;
        Ast *func;
        Type *output_type;
    } ops[16];

    enum { SINK_FOLD, SINK_LIST, SINK_ARRAY, SINK_FOR_EACH, SINK_COROUTINE } sink;
    Ast *sink_func;   // for fold
    Ast *sink_init;   // for fold
    Type *result_type;
} SeqPipeline;
```

### How List and Coroutine unify

Both are valid **sources** and **sinks** of the same pipeline. The pipeline operates on the abstract sequence of elements flowing through, regardless of representation.

```
-- List -> List
[1,2,3] |> map f |> map g |> filter p
  = Pipeline(Source.List([1,2,3]), [Map f, Map g, Filter p], Sink.CollectList)

-- List -> scalar
[1,2,3] |> map f |> fold (+) 0
  = Pipeline(Source.List([1,2,3]), [Map f], Sink.Fold((+), 0))

-- Coroutine -> Coroutine
cor |> cor_map f |> cor_map g
  = Pipeline(Source.Coroutine(cor), [Map f, Map g], Sink.YieldCoroutine)

-- Cross-representation: List -> Coroutine
[1,2,3] |> cor_of_list |> cor_map f
  = Pipeline(Source.List([1,2,3]), [Map f], Sink.YieldCoroutine)

-- Cross-representation: Coroutine -> List
cor |> cor_to_list |> map f
  = Pipeline(Source.Coroutine(cor), [Map f], Sink.CollectList)
```

## Rewrite Rules

Once in pipeline form, optimizations are local pattern matches on the ops array:

```
-- Map fusion: adjacent maps compose
[... Map f, Map g ...] -> [... Map (g . f) ...]

-- Map-fold fusion: map absorbed into fold
[... Map f], Fold k z -> [...], Fold (\acc x -> k acc (f x)) z

-- Map-filter reorder: push filters toward source (reduces work)
[... Map f, Filter p ...] -> [... Filter (p . f), Map f ...]
  (valid because the language is pure)

-- Identity elimination
[... Map id ...] -> [...]

-- Source-sink cancellation
Source(List xs), [], Sink(CollectList) -> xs
```

Function composition uses the existing `codegen_compose_functions` in `function.c`.

## Lowering Strategies

After rewrite rules are applied, lower the remaining pipeline to concrete code. The choice depends on source/sink/ops combination:

### Option A: Direct loop (best for pure map chains, known-length sources)

```
Pipeline(Source.List(xs), [Map (g . f)], Sink.CollectList)
```
becomes a single traversal with direct list construction (no reverse needed):
```llvm
for each node in xs:
    result = append(g(f(node.data)))
```

### Option B: Coroutine pipeline (best when filter/flatmap is involved)

```
Pipeline(Source.List(xs), [Map f, Filter p], Sink.CollectList)
```
becomes:
```
cor_to_list(cor_filter(p, cor_map(f, cor_of_list(xs))))
```
Then existing `fuse_cor_maps` + LLVM coro-elide optimize further.

### Option C: Fold loop (when sink is Fold, zero allocation)

```
Pipeline(Source.List(xs), [Map (g . f)], Sink.Fold(k, z))
```
becomes:
```llvm
acc = z
for each node in xs:
    acc = k(acc, g(f(node.data)))
return acc
```

### Lowering decision matrix

| Source | Ops | Sink | Strategy |
|--------|-----|------|----------|
| List/Array | Map only | CollectList | Direct loop (no reverse) |
| List/Array | Map only | Fold | Single fold loop |
| Any | Contains Filter | CollectList | Coroutine pipeline |
| Any | Contains FlatMap | Any | Coroutine pipeline |
| Coroutine | Any | YieldCoroutine | Coroutine pipeline |
| Coroutine | Map only | CollectList | Direct loop over resume |

## Implementation Plan

### Phase 1: Detection

In `codegen_application` (or a new pre-codegen pass), when a call to a known function is seen (`map`, `filter`, `fold`, `cor_map`, `cor_filter`, etc.), walk the nested applications and try to build a `SeqPipeline`.

Functions to recognize (by module-qualified name or symbol tag):
- `Lists.map` -> Source from arg, Op.Map from function arg
- `Lists.filter` -> Source from arg, Op.Filter from function arg
- `Lists.fold` -> Sink.Fold, source from list arg
- `Lists.print_list` -> Sink.ForEach
- `cor_map` -> Op.Map
- `cor_filter` -> Op.Filter
- `cor_of_list` -> boundary: List source into coroutine pipeline
- `cor_to_list` -> boundary: coroutine pipeline into List sink
- `cor_loop` -> Sink.ForEach

### Phase 2: Optimization

Apply rewrite rules on the `SeqPipeline.ops` array:
- Collapse adjacent OP_MAP entries using `codegen_compose_functions`
- Absorb trailing maps into fold sinks
- Push filters toward source

### Phase 3: Lowering

Based on the decision matrix, emit either:
- A direct loop (new codegen, avoids reverse by building in-order)
- A coroutine pipeline (reuse existing cor_map/cor_filter infrastructure)
- A fold loop (straightforward iteration)

### Phase 4: Integration with escape analysis

For pipelines that lower to direct loops with CollectList sink, the output list can potentially be stack-allocated if escape analysis determines it doesn't escape. The pipeline IR makes this easier because the allocation site is explicit in the lowering.

## Relationship to Existing Code

- **Replaces** `fuse_cor_maps` in `coroutine_extensions.c` (subsumed by the general framework)
- **Uses** `codegen_compose_functions` in `function.c` for map fusion
- **Uses** existing coroutine infrastructure for filter/flatmap lowering
- **Extends** escape analysis in `escape_analysis.c` for stack allocation of pipeline results
- **Detection** sits in `codegen_application` in `application.c` (or a new pre-codegen pass)

