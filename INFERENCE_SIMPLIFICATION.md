# Inference Simplification Plan

## Goal

Make the type inference system easier to understand, easier to debug, and less fragile by separating the core type inference algorithm from language-specific typing features.

This plan is intentionally architectural rather than code-level. The objective is to define clearer boundaries, reduce hidden mutation and special-case behavior, and give the inference engine a shape that is easier to reason about.

The immediate target is not to redesign the language or remove advanced features. The target is to preserve behavior while making the implementation smaller, more explicit, and more modular.

## Current Problems

The system currently works, but the implementation is difficult to reason about because core inference logic and feature-specific logic are interleaved.

The main pressure points are:

- `infer()` acts as both a dispatcher and a policy hub.
- `infer_application()` contains generic application inference plus many feature-specific branches.
- `unify()` is responsible for more than structural unification.
- `solve_constraints()` is doing substitution management, partial resolution, conflict handling, and feature-specific fallback behavior.
- `Type` nodes carry several different kinds of meaning at once.
- substitution and AST annotation are applied in multiple places with unclear ownership.

This makes it hard to answer simple questions like:

- Which function is responsible for actual type equality?
- Where should typeclass obligations be resolved?
- When is a substitution considered final?
- Which functions are allowed to mutate types in place?
- Why does a lambda or application sometimes have stale type annotations?

## Target Shape

The desired architecture should have three clear layers.

### 1. Core HM Inference

This layer should be small and predictable. It should be responsible for:

- inferring expression types
- generating constraints
- unifying types structurally
- building substitutions
- generalizing and instantiating schemes

Core functions should have narrow contracts and should not know about most language features.

### 2. Feature Overlays

Language-specific typing behavior should sit on top of the core rather than inside it.

Examples:

- trait/typeclass resolution
- closure capture metadata
- recursive function/reference typing
- coroutine typing
- constructor application
- compile-time const typing

These should be expressed through explicit helper layers or dedicated modules, not as special cases scattered across core functions.

### 3. Finalization / Normalization

After inference and solving, the system should have one clear place where inferred results are normalized and attached back to AST nodes.

This phase should be responsible for:

- applying final substitutions
- resolving deferred obligations
- finalizing closure metadata
- finalizing AST type annotations

That makes the mutation model easier to understand.

## Architectural Principles

The following principles should guide any refactor.

### Principle 1: Core functions should have narrow contracts

Each key function should do one thing:

- `infer_expr`: infer and generate obligations
- `unify`: structural compatibility
- `solve`: build final substitutions / resolve deferred obligations
- `finalize`: apply substitutions back into the AST and outputs

If a function needs to understand five language features to do its job, it is probably too high-level or responsible for too much.

### Principle 2: Feature logic should not leak into core unification

Unification should not be a general policy engine.

Today `unify()` is handling:

- structural unification
- closure metadata propagation
- typeclass-related branching
- recursive-ref cases
- concrete type promotion

This makes `unify()` difficult to trust because its behavior depends on non-local policy.

The long-term goal should be:

- `unify()` handles structural equality of type expressions
- feature systems introduce obligations before or after unification
- feature systems are solved separately

### Principle 3: Equality constraints and trait obligations should be distinct

The system currently mixes “these two types must be equal” with “this type must implement a trait” in the same control flow.

These are different kinds of facts.

Equality constraints should look like:

- `t1 ~ t2`

Trait obligations should look like:

- `HasTrait(t, Arithmetic)`

These can still be represented using the current data structures initially, but they should be separated conceptually and in API boundaries.

### Principle 4: Substitution flow should be explicit

The system currently applies substitutions:

- inside the solver
- in lambda inference
- in application inference
- in AST rewrite helpers
- via environment updates

That creates ambiguity about which types are current and which are stale.

The system should instead define one rule:

- inference generates types and constraints
- solving returns substitutions
- one explicit finalization step applies those substitutions to outputs and annotations

### Principle 5: AST annotation should happen at clear boundaries

Node types should not be incidentally updated by arbitrary helper calls.

A cleaner model is:

- infer returns inferred type values
- finalization writes types onto AST nodes
- AST rewrite helpers are only used in well-defined post-solve phases

This reduces the chance of stale annotations and unexpected coupling.

## Main Areas to Simplify

## A. `infer()` as Dispatcher

### Current concern

`infer()` is both:

- the main expression dispatcher
- a place where various side effects and feature integrations are coordinated

This makes it hard to read the main inference flow as a tree-walk.

### Target

Keep `infer()` as a thin dispatcher and move branch-specific logic into dedicated helpers with explicit contracts.

For example:

- `infer_identifier`
- `infer_let_binding`
- `infer_lambda`
- `infer_application`
- `infer_match_expression`
- `infer_record_access`
- `infer_module_expr`

This is partly true already, but some helpers still contain too much policy logic.

### Implementation direction

- ensure each helper returns a type and only the expected side effects
- document whether each helper may mutate `ctx`, mutate AST annotations, or introduce obligations
- make `infer()` itself as boring as possible

## B. `infer_application()` is doing too much

### Current concern

`infer_application()` handles:

- normal function application
- coroutine application
- constructor application
- builtin special cases like `iter`
- special coroutine transforms like `cor_zip`
- primitive short-circuits
- constant-propagation-related typing behavior

This means the most common inference case is buried inside feature-specific behavior.

### Target

Split application inference into clear stages:

1. classify the callee/application kind
2. dispatch to the relevant typing path
3. run post-application annotation/finalization

Suggested helper structure:

- `classify_application(ast, func_type)`
- `infer_regular_fn_application(...)`
- `infer_builtin_application(...)`
- `infer_constructor_application(...)`
- `infer_coroutine_application(...)`
- `apply_application_post_effects(...)`

### Benefits

- normal function application becomes easy to read
- builtins stop polluting the central algorithm
- adding a new special application case becomes local

## C. `unify()` should become structural

### Current concern

`unify()` currently does several unrelated things:

- checks structural compatibility
- injects constraints
- propagates typeclass obligations
- handles recursive-ref special cases
- merges or copies closure metadata
- special-cases concrete-vs-resolve interactions

This makes it difficult to understand whether a failure is a structural mismatch or a feature-policy mismatch.

### Target

Refactor toward:

- a small structural unifier
- tiny pre-checks or post-checks for special representations

Possible decomposition:

- `unify_core(t1, t2, out)`
- `try_unify_recursive_refs(t1, t2, out)`
- `try_unify_closure_shapes(t1, t2, out)`
- `register_trait_requirements(t1, t2, out)`

The ideal end state is that feature hooks are not inside the main structural recursion.

### Important note

Recursive refs may be the hardest thing to remove from `unify()`. If they must stay, isolate them into a clearly named helper called from the top of `unify()` rather than keeping them inline among all other cases.

## D. `solve_constraints()` is acting as a policy engine

### Current concern

The solver currently:

- maintains substitutions
- applies substitutions incrementally
- merges typeclass resolve nodes
- handles concrete specialization of trait-related nodes
- can re-enter unification
- handles promotion fallback

It is not just solving equalities; it is interpreting language policy.

### Target

Separate the solver into conceptual phases:

1. solve equality constraints
2. resolve trait obligations
3. apply promotion policy if applicable
4. finalize substitutions

If this is too large a change up front, an intermediate step is still valuable:

- isolate all typeclass-related logic into one module
- ensure `solve_constraints()` delegates trait-specific work instead of owning it directly

### Desired effect

The solver should become easier to describe in one sentence.

Current description:

- “It updates substitutions while trying to reconcile typeclass-resolve nodes and fallback promotion and may also call unify.”

Target description:

- “It solves equality constraints, then resolves remaining trait obligations.”

If the second sentence is not true yet, the system is still too entangled.

## E. `Type` currently carries multiple concerns

### Current concern

A single `Type` node currently carries:

- type syntax
- type schemes
- trait/typeclass resolution placeholders
- closure metadata
- recursive-ref markers
- backend-related attrs and meta

This means basic helpers like substitution and equality cannot stay simple.

### Target

Separate concerns where practical.

Strong candidates:

- ordinary type expressions
- type schemes
- trait obligations
- closure metadata
- backend attrs/constness metadata

This does not need to become a fully separate object model immediately. A practical incremental strategy is to preserve the `Type` struct while treating some fields as belonging to dedicated subsystems with clearly defined entry points.

### Most useful first split

The most valuable conceptual split would be:

- type equality data
- obligation data

That would reduce the amount of inference logic that has to inspect `T_TYPECLASS_RESOLVE` as if it were a normal type constructor.

## F. Closure typing should be more local

### Current concern

Closures are currently modeled by attaching `closure_meta` directly to function types. This is pragmatic, but it forces general-purpose operations like unification and substitution to care about closure information.

### Target

Two viable directions exist.

#### Option 1: Keep `closure_meta`, isolate its handling

This is the lower-risk path.

- ordinary function unification stays ordinary
- closure metadata compatibility is checked in a thin wrapper
- closure construction/finalization is handled in closure-specific helpers

#### Option 2: Distinguish closure values from plain functions in inference

This is cleaner conceptually but more invasive.

- introduce a separate inferred value representation for closures
- erase that distinction later for codegen or final typing

### Recommendation

Use Option 1 first. The current representation can survive, but it should stop leaking into unrelated parts of the inference pipeline.

## G. Typeclass resolution should stop looking like ordinary types

### Current concern

Arithmetic and trait resolution are encoded through `T_TYPECLASS_RESOLVE`, and this representation is manipulated in:

- unification
- substitution
- constraint solving
- promotion logic

That means unresolved obligations are traveling through the system disguised as types.

### Target

Move toward a model where:

- type variables remain ordinary type variables
- trait requirements are explicit obligations

For example, `+` could generate:

- `lhs ~ a`
- `rhs ~ b`
- `Arithmetic(a)`
- `Arithmetic(b)`
- result relation according to promotion policy

This is much easier to reason about than recursive resolve nodes embedded in type syntax.

### Transitional strategy

If replacing `T_TYPECLASS_RESOLVE` immediately is too disruptive:

- keep it as the internal representation temporarily
- move every operation on it into one dedicated module
- forbid unrelated code from branching directly on `T_TYPECLASS_RESOLVE`

That alone would improve local reasoning.

## Recommended Refactor Phases

## Phase 0: Freeze Behavior and Add Characterization Tests

Before architectural changes:

- add tests for core inference behavior
- add tests for known edge cases
- add tests for trait resolution behavior
- add tests for closure capture behavior
- add tests for application typing and partial application

This is necessary because much of the current logic depends on subtle interactions.

The goal is not just coverage. The goal is to preserve behavior while changing structure.

## Phase 1: Document Contracts

Add short function-level documentation above key inference helpers:

- what they take
- what they return
- whether they mutate `ctx`
- whether they mutate AST nodes
- whether they add constraints
- whether they may resolve obligations

Functions that especially need this:

- `infer`
- `infer_application`
- `infer_fn_application`
- `infer_lambda`
- `unify`
- `solve_constraints`
- `apply_substitution`
- `generalize`
- `instantiate`

This is the lowest-risk improvement and will pay off immediately.

## Phase 2: Make Mutation Explicit

Clarify whether the following are pure or mutating:

- `apply_substitution`
- `resolve_type_in_env`
- `generalize`
- `instantiate`

Right now the answer is not always obvious from the call site.

Choose a consistent discipline:

- either make these functions return fresh values
- or explicitly document and rename mutating variants

Example pattern:

- `apply_substitution_copy(...)`
- `apply_substitution_in_place(...)`

Even if you do not implement both, naming one behavior clearly helps.

## Phase 3: Split Application Inference

Refactor `infer_application()` into smaller helpers without changing semantics.

Suggested order:

1. extract builtin/special-form classification
2. extract normal function application
3. extract post-application attribute propagation
4. isolate constructor/coroutine-specific paths

This is likely the best first structural refactor because the boundary is clear and the conceptual benefit is large.

## Phase 4: Isolate Typeclass Logic

Move typeclass-related logic out of:

- `unify()`
- `solve_constraints()`
- `apply_substitution()`

and into a dedicated trait-resolution module.

Initial goal:

- all direct branching on `T_TYPECLASS_RESOLVE` happens in one place

Long-term goal:

- replace `T_TYPECLASS_RESOLVE` with explicit obligations

This phase is one of the highest-value changes for understandability.

## Phase 5: Shrink `unify()`

Once trait logic is isolated, simplify `unify()` to mostly structural recursion:

- variable-variable
- variable-concrete
- function-function
- constructor-constructor
- primitive-primitive

Anything feature-specific should be delegated through explicitly named hooks.

A good success criterion:

- a new contributor can read `unify()` and understand its behavior without knowing about coroutines, closures, traits, or codegen attributes

## Phase 6: Normalize Constraint Representation

Move from one ad hoc constraint flow toward separate representations for:

- equality constraints
- trait obligations
- feature obligations if needed

Even if the implementation still stores them in linked lists, the API should distinguish them.

This will make the solver easier to redesign later.

## Phase 7: Centralize Finalization

After the previous refactors, introduce a clear finalization step that:

- applies final substitutions
- resolves remaining obligations
- updates AST node annotations
- finalizes closure metadata if needed

This reduces the amount of “patch-up” work happening inside local inference helpers.

## Suggested Module Boundaries

The codebase does not need a huge rewrite, but the responsibilities should become clearer.

Suggested module breakdown:

- `inference_core.c`
  - infer dispatcher
  - generalize / instantiate
  - environment interaction

- `unify_core.c`
  - structural unification only

- `constraint_solver.c`
  - equality solving
  - substitution production

- `trait_solver.c`
  - trait obligations
  - promotion/ranking policy

- `application_inference.c`
  - application classification
  - regular function application
  - builtin application hooks

- `closure_typing.c`
  - closure capture
  - closure metadata compatibility

- `inference_finalize.c`
  - apply final substitutions
  - annotate AST types
  - resolve deferred final typing state

The exact filenames are less important than the responsibilities.

## Coding Rules for the Refactor

To prevent the system from drifting back into entanglement, use these rules.

- `unify()` must not inspect AST.
- application inference must not know solver internals.
- solver code must not know builtin names like `iter` or `cor_zip`.
- backend attrs and compile-time const logic should not live in core equality logic.
- trait/typeclass solving should be isolated from plain type equality.
- closure handling should not silently mutate unrelated function types.
- AST annotation should happen at explicit boundaries.

These rules are more important than the exact implementation details.

## Recommended First Concrete Steps

If starting tomorrow with minimal risk, do this:

1. add function-level contract comments to the current inference pipeline
2. extract `infer_application()` into smaller helpers without semantic changes
3. isolate all `T_TYPECLASS_RESOLVE` handling behind a small internal API
4. simplify `unify()` after trait logic is isolated
5. centralize final substitution and AST finalization

This order gives the biggest readability gains while minimizing the chance of breaking behavior.

## Main Risks

## Risk 1: Full rewrite too early

The biggest danger is attempting to replace the current inference engine wholesale.

The system has accumulated real language behavior in subtle places. A full rewrite would likely regress edge cases and make debugging harder.

Preferred strategy:

- preserve behavior
- extract helpers
- define contracts
- isolate side effects
- simplify representation after boundaries are clear

## Risk 2: Improving names without reducing coupling

Renaming functions or moving files is not enough if the same hidden cross-cutting behavior remains.

Success should be measured by:

- fewer feature-specific branches in core functions
- clearer contracts
- fewer incidental mutations
- easier debugging of a type mismatch

## Risk 3: Mixing transitional and final designs

Some intermediate representations may remain temporarily awkward.

That is acceptable, but each phase should move the system in one direction:

- toward explicit obligations
- toward structural unification
- toward centralized finalization

Avoid introducing new special cases in the old style while refactoring.

## Success Criteria

The system is simpler when the following become true:

- `unify()` can be explained in a few sentences.
- `infer_application()` has a readable normal-case path.
- trait resolution is isolated from plain type equality.
- substitutions are applied in one clearly defined phase.
- AST type annotations are not patched opportunistically from many places.
- a contributor can debug a type mismatch without understanding every language feature.

## Final Recommendation

Do not start with representation changes alone. Start with boundaries and contracts.

The highest-leverage sequence is:

1. document behavior
2. extract helpers
3. isolate typeclass logic
4. shrink unification
5. centralize finalization
6. only then reconsider deeper representation changes

That approach should make the whole system easier to understand without forcing a destabilizing rewrite.

## Current State Addendum

This section summarizes the current inference implementation after the recent simplification work. It is intended as a checkpoint: what has improved, what is still incomplete, and which performance/lifetime issues are now visible more clearly.

### What Is Simpler Now

The current implementation is already closer to the target shape than the original system:

- inference is mostly routed through dedicated helpers such as `infer_application`, `infer_lambda`, `infer_match_expression`, `infer_identifier`, and `infer_let_expr`
- equality constraints and trait-related obligations are now represented separately
- trait/promotion-style logic is expressed through `Predicate` obligations rather than being fully embedded in `T_TYPECLASS_RESOLVE`
- final substitution application to AST annotations is centralized in `finalize_ast_types`
- builtin polymorphism is expressed through `TypeEnv + scheme_vars + predicates`

These are real architectural improvements and should be preserved while continuing to simplify the implementation.

## Outstanding Correctness / Design Issues

### 1. Recursive functions are only partially integrated

Named lambdas now support recursive self-reference by prebinding the lambda name to a fresh provisional type during `infer_lambda`, then constraining that provisional type to the inferred function type.

This is the right simplified shape for monomorphic recursion, but the surrounding design is still incomplete:

- recursive support is only wired through named lambdas, not general recursive lets
- recursive bindings are not yet clearly documented as monomorphic within their own definition
- recursion is still not represented as a first-class policy boundary in the inference contracts

The intended rule should remain:

- recursive bindings introduce a provisional monotype before inferring the RHS
- the RHS is inferred under that binding
- the provisional type is unified with the inferred RHS type
- generalization, if any, only happens after the RHS is complete

### 2. Match inference now works structurally, but its invariants are still fragile

`infer_match_expression` now:

- infers the scrutinee
- binds each pattern against the scrutinee type in a branch-local environment
- constrains each branch body to a shared result type

This is the right core shape.

However, match remains an area where representation details can still leak:

- the AST stores branches as a flat `pattern, body, pattern, body` array, so every traversal must remember that `len` counts branches while storage uses `len * 2`
- branch-local type annotations depend on post-solve finalization being applied consistently to every branch node
- guard clauses and constructor patterns are working, but they still rely on the pattern binder understanding concrete AST lowering details

This is better than before, but the implementation is still more representation-sensitive than ideal.

### 3. Pattern binding has become the hidden feature hub

`bind_pattern(...)` is now doing useful work for:

- let destructuring
- lambda parameters
- match patterns
- tuple/list patterns
- nullary constructors
- constructor-style application patterns
- match guards

This is a good consolidation, but it also means the pattern binder is becoming a new feature concentration point. If more pattern forms are added without care, the same complexity that used to live in `infer()` / `unify()` will simply migrate into `bind_pattern(...)`.

The desired contract should be explicit:

- `bind_pattern(pattern, expected_type, ctx)` only generates constraints and env bindings
- it should not perform unrelated finalization or ad hoc policy work
- constructor-specific logic should stay narrow and ideally route through constructor metadata rather than AST-shape special cases

### 4. Builtin polymorphism is cleaner, but still transitional

The current `TypeEnv`-based builtin scheme representation is much better than the older `T_SCHEME`-heavy approach, but the system is still in a mixed state:

- builtin and env polymorphism use `TypeEnv + scheme_vars + predicates`
- some external callers still use `T_SCHEME`
- compatibility wrappers still exist for generalization/instantiation

This means there are still two overlapping polymorphism representations in the codebase.

The long-term target should be:

- one polymorphic binding representation for inference-time values
- one clearly isolated compatibility layer for older external modules until they are migrated

### 5. `Comparable` is the right obligation shape, but the solver policy is still centralized

Renaming the witness-style obligation to `Comparable` clarified the semantics:

- unary obligations: `Trait(t : Eq)` / `Trait(t : Ord)` / `Trait(t : Arithmetic)`
- relational obligations: `Comparable(trait, lhs, rhs, witness)`

This is a much better model than hiding everything inside type syntax.

However, resolution policy still lives centrally in `resolve_predicates(...)`:

- ranking by concrete typeclass rank
- arithmetic-specific deferral rules
- Eq/Ord witness pushback into generic operands

That policy works, but it is still hardcoded in the typechecker rather than delegated to trait-specific logic.

The long-term direction should remain:

- traits own their comparison / witness resolution policy
- the solver only coordinates obligation solving
- `Comparable` remains the shared obligation shape, but not the place where all policy is hardcoded

### 6. Finalization is improved, but still easy to under-apply

`finalize_ast_types(...)` is now the clear post-solve AST rewrite phase. That is a major improvement.

The remaining issue is that every AST form that can contain inferred subexpressions must be visited correctly, and missing one traversal causes stale or generic annotations to survive. Recent bugs around `AST_FMT_STRING` and `AST_MATCH` branch traversal are examples of this.

The design implication is:

- finalization is now the right boundary
- but the AST traversal must be treated as a complete structural walk, not an ad hoc patch list

## Performance Concerns

The current system is correct enough to optimize, but the main costs are still dominated by representation choices rather than allocator overhead alone.

### 1. Memory lifetime is not separated cleanly enough

The most important performance concern is not just “how much do we allocate?”, but “which allocations must survive beyond `infer(...)`?”

There are at least three relevant lifetimes:

#### Persistent / AST lifetime

These allocations must survive after inference completes:

- final `ast->type` annotations
- type nodes reachable from finalized AST annotations
- retained `TypeEnv` entries that stay attached to later compilation phases
- builtin type schemes and builtin predicate templates

#### Inference-session lifetime

These allocations only need to survive for one `infer(...)` invocation:

- `Constraint` list nodes
- accumulated instantiated `Predicate` obligation nodes in `ctx->predicates`
- `Subst` list nodes
- temporary `Comparable` argument arrays created during obligation copying / rewriting

These are strong candidates for separate allocators or a dedicated inference-session arena.

#### Scratch / solve-time temporary lifetime

These allocations are even shorter-lived and often only need to survive during one solver pass or one helper call:

- temporary rewritten `Type` nodes created by `apply_subst_to_type(...)`
- temporary predicate copies from `predicate_apply_subst(...)`
- temporary instantiated structures that are only used to derive the final substitution

At the moment these lifetimes are not cleanly separated enough. This makes memory profiling harder and causes temporary churn to look more persistent than it really is.

### 2. Separate allocators help memory behavior, but are not the main speed win

Giving `Constraint`, `Predicate`, and `Subst` their own inference-session allocators is still worthwhile because it:

- reduces lifetime confusion
- improves locality within each node kind
- makes teardown effectively free
- makes allocation profiling by category much easier

However, this is mostly a constant-factor improvement. It helps memory behavior directly and runtime somewhat, but it does not fix the dominant algorithmic costs.

### 3. The current substitution representation is the main solver bottleneck

The current substitution design uses:

- linked-list substitution nodes
- string-based type-variable identity
- repeated linear lookup by variable name

This affects:

- `lookup_subst(...)`
- `find_root_var(...)`
- `apply_subst_to_type(...)`
- `unify_types(...)`
- predicate resolution

The result is a lot of repeated linear work plus repeated string comparisons.

The most important medium-term speed improvement would be:

- give type variables a stable integer id
- store solver state indexed by that id
- replace linked-list substitution lookup with table-driven lookup, ideally union-find style

This is why adding an `id` to `T_VAR` is a useful preparatory step. It opens the path toward:

- array-indexed type-variable state
- path compression
- cheaper root lookup
- less repeated string handling

### 4. Constraint management is still too list-oriented

Constraints are currently maintained as a linked list, and insertion currently scans existing constraints to avoid exact duplicates.

This has two costs:

- poor cache locality from linked-list traversal
- O(n) duplicate checks during insertion

At current scale this may be acceptable, but it is not a good long-term shape.

Likely improvements:

- remove duplicate checking entirely unless measurements prove it is valuable
- or replace it with a hash-based dedup strategy
- eventually store constraints in a growable array / worklist rather than a linked list

### 5. Predicate rewriting is allocation-heavy

`predicate_apply_subst(...)` currently rewrites whole predicate lists and rebuilds comparable argument arrays.

That is not wrong, but it is a likely source of avoidable temporary allocation churn.

Possible future improvements:

- resolve predicates against the current substitution on demand rather than rewriting the whole list
- allocate rewritten predicates in scratch memory only
- reduce repeated arg-array rebuilding for small fixed-arity builtins

### 6. `apply_subst_to_type(...)` is a likely hot path

This helper is structurally correct and already preserves sharing where it can, but it still rebuilds function/constructor nodes whenever any child changes.

That means it is probably one of the main temporary allocation sources during solving and finalization.

It should be profiled specifically:

- number of calls per `infer(...)`
- number of new `Type` nodes allocated inside it
- how often the same logical type is rewritten repeatedly

If solving is slow, this function is a prime suspect.

## Recommended Performance Instrumentation

Before changing data structures aggressively, add measurements.

The most useful immediate counters are:

- number of `Type` allocations during one `infer(...)`
- number of `Constraint` allocations
- number of `Predicate` allocations
- number of `Subst` allocations
- number of calls to `apply_subst_to_type(...)`
- number of constraints solved
- number of predicate-resolution iterations

These counters should ideally be split by lifetime bucket:

- persistent
- inference-session
- scratch

Without that distinction, memory measurements will remain noisy and misleading.

## Practical Next Steps

The current priority order should be:

1. separate persistent allocations from inference-session allocations
2. add allocation and hot-path counters
3. stop paying obvious O(n) costs where they are not buying much
4. continue the move from string-based `T_VAR` identity toward integer-id-based lookup
5. only then consider deeper solver data-structure replacement

The most important strategic point is this:

- allocator separation is mainly about lifetime clarity and memory behavior
- the biggest speed wins will come from better type-variable and substitution data structures
- both should be designed together, but allocator/lifetime cleanup is the safer first step
