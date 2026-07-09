# REPL JIT Transaction Modules

This is a design note for moving the YLC REPL away from a single growing LLVM
module and toward small per-input modules. The goal is lower REPL latency, more
predictable JIT ownership, and a cleaner path to either MCJIT `addModule` or
ORC/LLJIT.

## Vocabulary

**MCJIT** is LLVM's older execution engine. It owns LLVM modules, compiles them
to native code, links them into the process, and lets the host look up addresses
for functions or globals. YLC currently uses this family through the LLVM C
ExecutionEngine API in `lang/backend_llvm/jit.c`.

**ORC** means "On-Request-Compilation". It is LLVM's newer JIT framework. It is
not one fixed JIT; it is a set of layers and symbol-resolution machinery for
building a JIT. ORC is the third generation LLVM JIT API after the deleted
legacy JIT and MCJIT.

**LLJIT** is the off-the-shelf ORC JIT that most closely matches MCJIT's role.
It accepts LLVM IR modules, compiles them, links them, and supports symbol
lookup. LLVM describes LLJIT as a suitable MCJIT replacement for most use cases.

**LLLazyJIT** extends LLJIT with lazy compilation. That is probably not the first
step for YLC's REPL, but it may become useful if larger scripts define many
functions that are not immediately called.

**JITDylib** is ORC's JIT-side symbol table and link unit. For a YLC REPL, a
single main JITDylib is probably enough initially. More JITDylibs become useful
if modules need isolated symbol namespaces.

**ResourceTracker** is ORC's ownership handle for removing JIT'd code and
associated resources. This is a major reason to prefer ORC once the basic
transaction model works.

## Current YLC Shape

The current REPL path is still based on one accumulated LLVM module:

- `codegen_repl_top_level` in `lang/backend_llvm/codegen.c` emits a function
  named `top` into the current module.
- REPL input is typechecked against the persistent YLC type environment.
- The backend codegen reuses the same LLVM module and symbol table.
- Recent changes run a smaller function-only optimization pipeline for REPL
  submissions and create the REPL MCJIT engine with a lower MCJIT opt level.
- Top-level YLC values are stored through `global_storage_array` in
  `lang/backend_llvm/globals.c`.

That already improved latency, but the architecture is still awkward. The REPL
mutates a module after it has been used for JIT execution, and each later REPL
submission carries the weight of all previous LLVM IR.

## Transaction Module Model

A transaction module is a fresh LLVM module for one REPL submission.

The persistent compiler state remains in C/YLC data structures:

- type environment
- symbol table
- global storage slots
- loaded dynamic libraries
- module imports
- generic specialization cache, if retained

The LLVM module for a single transaction contains only:

- a uniquely named entry function, for example `__ylc_repl_top_42`
- any helper functions or specialized functions generated for this transaction
- declarations for older functions it calls
- declarations for runtime functions and externs
- declarations for `global_storage_array` and `global_storage_size`

The key rule is that codegen for a new transaction must not directly reuse
`LLVMValueRef`s from an old module. Cross-transaction references should be
lowered by name, by global-storage slot, or by explicit runtime address.

## Why This Helps

Small transaction modules keep optimization and machine-code emission bounded by
the new input. This is the most direct latency win for notebook-style REPL use.

They also clarify ownership. Once a module has been handed to a JIT engine, YLC
should treat it as immutable and owned by the JIT. Future REPL inputs should
generate new modules instead of appending to the old one.

Finally, transactions are a better match for graph compilation later. A compiled
autograd graph, a CUDA/MLIR lowering, or a specialized tensor kernel can become
one transaction artifact with explicit inputs, outputs, dependencies, and
lifetime.

## Required Codegen Changes

The hard part is not creating a new LLVM module. The hard part is making symbol
lowering module-independent.

### Unique Top Functions

`codegen_repl_top_level` should stop emitting a fixed `top`. It should emit a
unique name per input, such as:

```text
__ylc_repl_top_0
__ylc_repl_top_1
__ylc_repl_top_2
```

The REPL then looks up exactly that symbol and calls it.

### Top-Level Variables

Top-level variables are already close to the desired design. A symbol for a
top-level value should mean:

```text
load slot N from global_storage_array and cast/load it as type T
```

This is better than carrying an old `LLVMValueRef` into a new module. The
transaction module only needs an external declaration of `global_storage_array`
with the same LLVM type.

### Top-Level Functions

Functions need a similar symbolic representation:

```text
function name + function type + linkage/import/export status
```

When a later transaction calls an earlier function, codegen should call
`LLVMGetNamedFunction` in the transaction module and create an external
declaration if it is missing. It should not reuse the original function's
`LLVMValueRef`.

### Generated Helpers

Any generated helper with a stable user-visible name can be exported. Any helper
that is purely internal to one transaction should have either:

- a unique generated name, or
- internal/private linkage.

This matters because duplicated helper names across transactions will collide
once all transactions live in the same JIT symbol namespace.

### Runtime and Extern Symbols

Runtime functions, C externs, and symbols from loaded libraries need to be
available to the JIT linker. With MCJIT this usually means process symbol lookup,
explicit global mappings, or dynamic-library loading. With ORC/LLJIT this should
be modeled with symbol generators or absolute symbols.

## MCJIT addModule Path

This is the smaller migration from the current implementation.

The model would be:

1. Create one MCJIT execution engine.
2. Add the initial script module.
3. For every REPL input, create a fresh transaction module.
4. Give it the same target triple and data layout.
5. Add declarations for referenced previous symbols and runtime globals.
6. Run a small pass pipeline on the transaction module.
7. Add it to the existing engine with `LLVMAddModule`.
8. Look up `__ylc_repl_top_N` with `LLVMGetFunctionAddress`.
9. Call the returned function pointer.

Pros:

- smaller change from current C API usage
- likely enough to remove most REPL latency from large accumulated modules
- easier stepping stone before ORC

Cons:

- MCJIT is the older API generation
- code removal and per-transaction lifetime control are weak
- symbol behavior is less explicit than ORC
- modules should be treated as immutable after being added to the engine

## ORC/LLJIT Path

This is the cleaner long-term model.

The model would be:

1. Create one `LLVMOrcLLJITRef`.
2. Use the main JITDylib as the REPL symbol namespace.
3. Add the initial script module as a `ThreadSafeModule`.
4. For every REPL input, create a new transaction module.
5. Add it with `LLVMOrcLLJITAddLLVMIRModule` or
   `LLVMOrcLLJITAddLLVMIRModuleWithRT`.
6. Look up `__ylc_repl_top_N` with `LLVMOrcLLJITLookup`.
7. Call the returned address as a function pointer.

Pros:

- LLVM's current JIT architecture
- designed for REPLs, interpreters, and composable JITs
- explicit JIT symbol tables through JITDylibs
- clear ownership of modules and JIT'd code
- ResourceTrackers can remove code later
- natural path to lazy compilation or concurrent compilation

Cons:

- larger migration
- requires ThreadSafeModule/ThreadSafeContext plumbing
- symbol mangling and process symbol setup are more explicit
- the C ORC API is more verbose than MCJIT's ExecutionEngine API

## Recommended Migration Path

1. Keep the current faster REPL path as the baseline.
2. Make REPL top-level function names unique.
3. Introduce a module-independent symbol-lowering layer:
   - top-level vars lower through `global_storage_array` slots
   - functions lower through named declarations
   - externs lower through named declarations plus runtime/JIT symbol setup
4. Create transaction modules while still using the existing MCJIT approach for
   experiments.
5. Move to one MCJIT engine plus `LLVMAddModule`.
6. Once that works, port the same transaction module interface to LLJIT.
7. Add ResourceTrackers only after basic LLJIT add/lookup/call is stable.

The important design boundary is step 3. If symbols can be lowered into any
module on demand, the choice between MCJIT `addModule` and ORC/LLJIT becomes
mostly a JIT backend decision.

## Suggested Experiments

- Submit `1 + 1` as a transaction and call `__ylc_repl_top_0`.
- Define `let x = 10;;`, then submit `x + 1` in a new transaction.
- Define a function in the base script, then call it from a later transaction.
- Define a function in one REPL transaction, then call it from the next.
- Load a C extern and call it from a transaction module.
- Submit two transactions that both need generated helper functions and verify
  that symbol names do not collide.
- Print or dump each transaction module before adding it to the JIT.

## How This Relates to Autograd Graph Compilation

For autograd, the same idea can be used above the LLVM layer:

- flatten the graph into a normalized IR
- choose graph inputs, parameter tensors, outputs, and mutable gradients
- emit one transaction module for the graph or graph segment
- compile and cache it by graph shape/op signature
- call the compiled entry point from YLC

The transaction module should not depend on incidental heap addresses for graph
nodes. It should depend on explicit buffers, shapes, op IDs, and stable runtime
entry points.

## Research Links

- [LLVM ORC Design and Implementation](https://llvm.org/docs/ORCv2.html)
- [LLVM Building an ORC-based JIT tutorial](https://llvm.org/docs/tutorial/BuildingAJIT1.html)
- [LLVM MCJIT Design and Implementation](https://llvm.org/docs/MCJITDesignAndImplementation.html)
- [LLVM C ExecutionEngine API](https://llvm.org/doxygen/group__LLVMCExecutionEngine.html)
- [LLVM C LLJIT API](https://llvm.org/doxygen/group__LLVMCExecutionEngineLLJIT.html)
- [LLVM Kaleidoscope JIT and Optimizer chapter](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl04.html)
