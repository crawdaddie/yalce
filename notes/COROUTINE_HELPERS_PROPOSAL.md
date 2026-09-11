# Coroutine Helper Functions Proposal

## Problem
Lots of boilerplate in `coroutines.c` and `coroutine_extensions.c` that repeats:
- Initial suspend setup (~15-20 lines)
- Yield point creation (~25-30 lines)
- Final suspend (~15-20 lines)
- Cleanup & suspend blocks (~10-15 lines)
- Yield-from loops (~40-50 lines)

## Solution
Simple helper functions that work with existing `CoroutineCtx*` and builder state.

---

## Proposed Helper Functions

### 1. Initial Suspend Helper

**Currently repeated everywhere:**
```c
LLVMValueRef initial_save = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
    get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1, "initial.save");

LLVMValueRef initial_suspend = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
    get_coro_suspend_intrinsic(module),
    (LLVMValueRef[]){initial_save, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
    "initial.suspend");

LLVMValueRef init_switch = LLVMBuildSwitch(builder, initial_suspend, initial_return_bb, 2);
LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), start_bb);
LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

LLVMPositionBuilderAtEnd(builder, initial_return_bb);
LLVMBuildBr(builder, suspend_bb);
```

**Becomes:**
```c
/**
 * Emit initial suspend boilerplate
 * - Must be positioned at entry block
 * - Emits coro.save + coro.suspend + switch
 * - Positions builder at start_bb when done
 *
 * Requires: entry_bb, cleanup_bb, suspend_bb, initial_return_bb, start_bb defined
 */
void emit_coro_initial_suspend(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMValueRef handle,
    LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb,
    LLVMBasicBlockRef initial_return_bb,
    LLVMBasicBlockRef start_bb
);
```

Usage:
```c
emit_coro_initial_suspend(builder, module, handle, cleanup_bb, suspend_bb,
                          initial_return_bb, start_bb);
// Now positioned at start_bb, ready for body
```

---

### 2. Yield Point Helper

**Currently repeated per yield:**
```c
LLVMBuildStore(builder, value, promise_alloca);

LLVMValueRef save_token = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
    get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1, "coro.save");

LLVMValueRef suspend_result = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
    get_coro_suspend_intrinsic(module),
    (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "coro.suspend");

LLVMBasicBlockRef return_bb = LLVMAppendBasicBlock(..., "yield.return");
LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(..., "yield.resume");

LLVMValueRef switch_inst = LLVMBuildSwitch(builder, suspend_result, return_bb, 2);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

LLVMPositionBuilderAtEnd(builder, return_bb);
LLVMBuildBr(builder, suspend_bb);

LLVMPositionBuilderAtEnd(builder, resume_bb);
```

**Becomes:**
```c
/**
 * Emit a yield point
 * - Stores value to promise
 * - Emits save + suspend + switch
 * - Creates yield.return and yield.resume blocks
 * - Positions builder at resume block when done
 *
 * Returns: The resume block where execution continues
 */
LLVMBasicBlockRef emit_coro_yield(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMValueRef handle,
    LLVMValueRef promise_alloca,
    LLVMValueRef value,
    LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb,
    int yield_index  // For unique block naming
);
```

Usage:
```c
emit_coro_yield(builder, module, handle, promise_alloca, value1,
                cleanup_bb, suspend_bb, 0);
// Continue with next statement
emit_coro_yield(builder, module, handle, promise_alloca, value2,
                cleanup_bb, suspend_bb, 1);
```

---

### 3. Final Suspend Helper

**Currently repeated everywhere:**
```c
LLVMValueRef final_save = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
    get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1, "final.save");

LLVMValueRef final_suspend = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
    get_coro_suspend_intrinsic(module),
    (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
    "final.suspend");

LLVMBasicBlockRef final_return_bb = LLVMAppendBasicBlock(coro_fn, "final.return");
LLVMValueRef final_switch = LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), final_return_bb);
LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

LLVMPositionBuilderAtEnd(builder, final_return_bb);
LLVMBuildBr(builder, suspend_bb);
```

**Becomes:**
```c
/**
 * Emit final suspend (marks end of coroutine)
 * - Emits save + suspend(final=true) + switch
 * - Creates final.return block
 * - Connects to suspend_bb
 */
void emit_coro_final_suspend(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMValueRef handle,
    LLVMValueRef function,
    LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb
);
```

Usage:
```c
emit_coro_final_suspend(builder, module, handle, coro_fn, cleanup_bb, suspend_bb);
```

---

### 4. Cleanup & Suspend Blocks Helper

**Currently repeated everywhere:**
```c
// Cleanup block
LLVMPositionBuilderAtEnd(builder, cleanup_bb);
LLVMValueRef mem = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
    get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2, "coro.free");
LLVMBuildFree(builder, mem);
LLVMBuildBr(builder, suspend_bb);

// Suspend block
LLVMPositionBuilderAtEnd(builder, suspend_bb);
LLVMBuildCall2(builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
               get_coro_end_intrinsic(module),
               (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
LLVMBuildRet(builder, handle);
```

**Becomes:**
```c
/**
 * Emit cleanup and suspend blocks
 * - Cleanup: calls coro.free and free()
 * - Suspend: calls coro.end and returns handle
 */
void emit_coro_cleanup_and_suspend(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMValueRef coro_id,
    LLVMValueRef handle,
    LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb
);
```

Usage:
```c
emit_coro_cleanup_and_suspend(builder, module, id, handle, cleanup_bb, suspend_bb);
```

---

### 5. Yield-From Loop Helper

**Currently ~40-50 lines in multiple places:**

**Becomes:**
```c
/**
 * Emit a yield-from loop (for nested coroutines)
 * - Creates loop_check, loop_body, loop_resume, loop_exit blocks
 * - Emits: check if inner done → resume inner → read promise → yield value → loop
 * - Positions builder at loop_exit when inner exhausted
 *
 * For use in cor_loop, cor_map, user yield-from, etc.
 *
 * Returns: The loop_exit block
 */
LLVMBasicBlockRef emit_coro_yield_from_loop(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMValueRef wrapper_handle,
    LLVMValueRef inner_handle,
    LLVMValueRef promise_alloca,
    LLVMTypeRef yield_type,
    LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb,
    const char* label_prefix  // "loop", "map", etc.
);
```

Usage:
```c
// In CorLoopHandler:
LLVMBasicBlockRef loop_exit = emit_coro_yield_from_loop(
    builder, module, handle, inner_handle, promise_alloca,
    yield_type, cleanup_bb, suspend_bb, "loop"
);
// positioned at loop_exit - can branch back for infinite loop
LLVMBuildBr(builder, loop_check_bb);  // Infinite loop!
```

---

### 6. Setup Helper (combines initialization)

**Convenience wrapper for common initialization pattern:**

```c
/**
 * Setup coroutine entry block with standard boilerplate
 * - Allocates promise
 * - Calls coro.id, coro.size, malloc, coro.begin
 * - Returns the initialized values
 *
 * Requires: Already positioned at entry_bb
 */
typedef struct {
  LLVMValueRef promise_alloca;
  LLVMValueRef coro_id;
  LLVMValueRef handle;
} CoroSetupResult;

CoroSetupResult emit_coro_setup(
    LLVMBuilderRef builder,
    LLVMModuleRef module,
    LLVMTypeRef promise_type
);
```

Usage:
```c
LLVMPositionBuilderAtEnd(builder, entry_bb);
CoroSetupResult setup = emit_coro_setup(builder, module, yield_type);

// Now use setup.promise_alloca, setup.coro_id, setup.handle
```

---

## Integration with Existing Code

### Before: cor_loop (220 lines)
```c
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, ...) {
  // ... type extraction (10 lines) ...

  // Create function (5 lines)
  LLVMValueRef wrapper_fn = LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Setup promise, id, handle (15 lines)
  LLVMValueRef promise_alloca = LLVMBuildAlloca(builder, llvm_yield_type, "promise");
  LLVMValueRef id = LLVMBuildCall2(...);
  // ... etc ...

  // Initial suspend (15 lines)
  LLVMValueRef initial_save = LLVMBuildCall2(...);
  LLVMValueRef initial_suspend = LLVMBuildCall2(...);
  // ... switch setup ...

  // Loop logic (80 lines)
  // ... complex loop with yield-from ...

  // Final suspend (15 lines)
  // Cleanup (10 lines)
  // ... etc ...
}
```

### After: cor_loop (~50 lines)
```c
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, ...) {
  // ... type extraction (10 lines) ...

  LLVMValueRef wrapper_fn = LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Setup (1 call)
  CoroSetupResult setup = emit_coro_setup(builder, module, llvm_yield_type);

  // Initial suspend (1 call)
  emit_coro_initial_suspend(builder, module, setup.handle, cleanup_bb,
                            suspend_bb, initial_return_bb, start_bb);

  // Infinite loop with yield-from
  LLVMBasicBlockRef infinite_loop = LLVMAppendBasicBlock(wrapper_fn, "infinite_loop");
  LLVMBuildBr(builder, infinite_loop);
  LLVMPositionBuilderAtEnd(builder, infinite_loop);

  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);
  LLVMBasicBlockRef loop_exit = emit_coro_yield_from_loop(
      builder, module, setup.handle, inner_handle, setup.promise_alloca,
      llvm_yield_type, cleanup_bb, suspend_bb, "loop"
  );
  LLVMBuildBr(builder, infinite_loop);  // Loop forever

  // Cleanup & suspend (1 call)
  emit_coro_cleanup_and_suspend(builder, module, setup.coro_id, setup.handle,
                                cleanup_bb, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return wrapper_fn;
}
```

---

## Implementation Plan

### Phase 1: Core Helpers (simplest first)
1. `emit_coro_setup` - Setup helper
2. `emit_coro_cleanup_and_suspend` - Cleanup helper
3. `emit_coro_initial_suspend` - Initial suspend
4. `emit_coro_final_suspend` - Final suspend

### Phase 2: Yield Helpers
5. `emit_coro_yield` - Single yield point
6. `emit_coro_yield_from_loop` - Yield-from loop

### Phase 3: Refactor Existing Code
7. Update `compile_coroutine` to use helpers
8. Update `CorLoopHandler` to use helpers
9. Update `CorMapHandler` to use helpers
10. Update `CorOfListHandler` to use helpers

---

## File Organization

```c
// In coroutines.h - add declarations:
CoroSetupResult emit_coro_setup(LLVMBuilderRef builder, LLVMModuleRef module,
                                LLVMTypeRef promise_type);

void emit_coro_initial_suspend(LLVMBuilderRef builder, LLVMModuleRef module,
                               LLVMValueRef handle, LLVMBasicBlockRef cleanup_bb,
                               LLVMBasicBlockRef suspend_bb,
                               LLVMBasicBlockRef initial_return_bb,
                               LLVMBasicBlockRef start_bb);

LLVMBasicBlockRef emit_coro_yield(LLVMBuilderRef builder, LLVMModuleRef module,
                                  LLVMValueRef handle, LLVMValueRef promise_alloca,
                                  LLVMValueRef value, LLVMBasicBlockRef cleanup_bb,
                                  LLVMBasicBlockRef suspend_bb, int yield_index);

void emit_coro_final_suspend(LLVMBuilderRef builder, LLVMModuleRef module,
                             LLVMValueRef handle, LLVMValueRef function,
                             LLVMBasicBlockRef cleanup_bb,
                             LLVMBasicBlockRef suspend_bb);

void emit_coro_cleanup_and_suspend(LLVMBuilderRef builder, LLVMModuleRef module,
                                   LLVMValueRef coro_id, LLVMValueRef handle,
                                   LLVMBasicBlockRef cleanup_bb,
                                   LLVMBasicBlockRef suspend_bb);

LLVMBasicBlockRef emit_coro_yield_from_loop(
    LLVMBuilderRef builder, LLVMModuleRef module, LLVMValueRef wrapper_handle,
    LLVMValueRef inner_handle, LLVMValueRef promise_alloca,
    LLVMTypeRef yield_type, LLVMBasicBlockRef cleanup_bb,
    LLVMBasicBlockRef suspend_bb, const char* label_prefix);


// In coroutine_helpers.c - new file with implementations
```

---

## Benefits

- **75-80% code reduction** in coroutine handlers
- **Works with existing CoroutineCtx** - no new abstractions
- **Simple function calls** - easy to understand
- **Easy to extend** - add cancellation checks to yield helpers later
- **No API churn** - just internal refactoring

## Next Steps

1. Should I implement these helpers in a new `coroutine_helpers.c` file?
2. Any specific helpers you'd want different signatures for?
3. Want to tackle them incrementally (starting with simplest helpers)?
