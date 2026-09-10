# Coroutine IR Abstraction Layer Proposal

## Current State: Identified Boilerplate Patterns

After analyzing `coroutines.c` and `coroutine_extensions.c`, I've identified these repeated patterns:

### 1. **Coroutine Initialization** (70-80 lines, repeated 5+ times)
```c
// Create function
LLVMValueRef fn = LLVMAddFunction(module, name, fn_type);
COROUTINE_ATTR_MARKING(fn)
COROUTINE_BASIC_BLOCKS(fn)

// Allocate promise
LLVMValueRef promise = LLVMBuildAlloca(builder, yield_type, "promise");

// Setup coroutine intrinsics
LLVMValueRef id = LLVMBuildCall2(..., get_coro_id_intrinsic(module), ...);
LLVMValueRef size = LLVMBuildCall2(..., get_coro_size_intrinsic(module), ...);
LLVMValueRef frame = LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");
LLVMValueRef handle = LLVMBuildCall2(..., get_coro_begin_intrinsic(module), ...);
```

### 2. **Initial Suspend** (15-20 lines, repeated everywhere)
```c
LLVMValueRef initial_save = LLVMBuildCall2(..., get_coro_save_intrinsic(module), ...);
LLVMValueRef initial_suspend = LLVMBuildCall2(..., get_coro_suspend_intrinsic(module), ...);
LLVMValueRef init_switch = LLVMBuildSwitch(builder, initial_suspend, initial_return_bb, 2);
LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), start_bb);
LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);
LLVMPositionBuilderAtEnd(builder, initial_return_bb);
LLVMBuildBr(builder, suspend_bb);
```

### 3. **Yield Point** (25-30 lines, repeated per yield)
```c
LLVMBuildStore(builder, value, promise_alloca);
LLVMValueRef save_token = LLVMBuildCall2(..., get_coro_save_intrinsic(module), ...);
LLVMValueRef suspend_result = LLVMBuildCall2(..., get_coro_suspend_intrinsic(module), ...);
LLVMBasicBlockRef return_bb = LLVMAppendBasicBlock(..., "yield.return");
LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(..., "yield.resume");
LLVMValueRef switch_inst = LLVMBuildSwitch(builder, suspend_result, return_bb, 2);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);
LLVMPositionBuilderAtEnd(builder, return_bb);
LLVMBuildBr(builder, suspend_bb);
LLVMPositionBuilderAtEnd(builder, resume_bb);
```

### 4. **Final Suspend** (15-20 lines, repeated everywhere)
```c
LLVMValueRef final_save = LLVMBuildCall2(..., get_coro_save_intrinsic(module), ...);
LLVMValueRef final_suspend = LLVMBuildCall2(..., get_coro_suspend_intrinsic(module),
    ..., LLVMConstInt(LLVMInt1Type(), 1, 0), ...); // true = FINAL
LLVMBasicBlockRef final_return_bb = LLVMAppendBasicBlock(..., "final.return");
LLVMValueRef final_switch = LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), final_return_bb);
LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);
LLVMPositionBuilderAtEnd(builder, final_return_bb);
LLVMBuildBr(builder, suspend_bb);
```

### 5. **Cleanup & Suspend** (10-15 lines, identical everywhere)
```c
// Cleanup block
LLVMPositionBuilderAtEnd(builder, cleanup_bb);
LLVMValueRef mem = LLVMBuildCall2(..., get_coro_free_intrinsic(module), ...);
LLVMBuildFree(builder, mem);
LLVMBuildBr(builder, suspend_bb);

// Suspend block
LLVMPositionBuilderAtEnd(builder, suspend_bb);
LLVMBuildCall2(..., get_coro_end_intrinsic(module), ...);
LLVMBuildRet(builder, handle);
```

### 6. **Yield-From Loop** (40-50 lines, repeated in cor_loop, cor_map, etc.)
```c
LLVMBasicBlockRef loop_check_bb = LLVMAppendBasicBlock(wrapper_fn, "check");
LLVMBasicBlockRef loop_body_bb = LLVMAppendBasicBlock(wrapper_fn, "body");
LLVMBasicBlockRef loop_resume_bb = LLVMAppendBasicBlock(wrapper_fn, "resume");
LLVMBasicBlockRef loop_exit_bb = LLVMAppendBasicBlock(wrapper_fn, "exit");

// Check if inner done
LLVMValueRef is_done = LLVMBuildCall2(..., get_coro_done_intrinsic(module), ...);
LLVMBuildCondBr(builder, is_done, loop_exit_bb, loop_body_bb);

// Resume inner
LLVMBuildCall2(..., get_coro_resume_intrinsic(module), ...);
// ... check done again, read promise, yield value, suspend, switch ...
```

---

## Proposed Abstraction: Coroutine Builder API

Create a higher-level API that treats coroutine construction like an IR. The key insight: **coroutines have a lifecycle with well-defined phases**.

### Core Data Structure

```c
// coroutine_builder.h

typedef struct CoroutineBuilder {
  // Function & Module
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef function;

  // Coroutine State
  LLVMValueRef coro_id;
  LLVMValueRef coro_handle;
  LLVMValueRef promise_alloca;
  LLVMTypeRef promise_type;

  // Standard Basic Blocks (created automatically)
  LLVMBasicBlockRef entry_bb;
  LLVMBasicBlockRef cleanup_bb;
  LLVMBasicBlockRef suspend_bb;
  LLVMBasicBlockRef initial_return_bb;
  LLVMBasicBlockRef start_bb;

  // Builder state
  LLVMBasicBlockRef prev_insert_block;
  int yield_counter;

} CoroutineBuilder;
```

### Phase 1: Creation & Initialization

```c
/**
 * Creates a new coroutine builder with all standard setup
 * - Creates function with proper attributes
 * - Creates standard basic blocks (entry, cleanup, suspend, etc.)
 * - Allocates promise storage
 * - Calls coro.id, coro.size, malloc, coro.begin
 * - Returns builder ready for body construction
 */
CoroutineBuilder* coro_builder_create(
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    const char* name,
    LLVMTypeRef promise_type,
    LLVMTypeRef* param_types,
    int param_count
);

/**
 * Emits initial suspend boilerplate
 * - Positions at entry block
 * - Emits coro.save + coro.suspend
 * - Creates switch with proper cases
 * - Positions builder at start_bb when done
 */
void coro_builder_emit_initial_suspend(CoroutineBuilder* cb);
```

### Phase 2: Body Construction

```c
/**
 * Emit a single yield point
 * - Stores value to promise
 * - Emits coro.save + coro.suspend
 * - Creates switch with resume/destroy/suspend cases
 * - Creates and connects proper basic blocks
 * - Positions builder at resume block for next statement
 *
 * Returns: The resume block (where execution continues)
 */
LLVMBasicBlockRef coro_builder_emit_yield(
    CoroutineBuilder* cb,
    LLVMValueRef value
);

/**
 * Emit a yield-from loop (for nested coroutines)
 * - Creates loop structure with check/body/resume/exit blocks
 * - Emits coro.done checks
 * - Emits coro.resume calls
 * - Reads promise and yields values
 * - Handles suspension properly
 *
 * Returns: The exit block (where execution continues after inner exhausted)
 */
LLVMBasicBlockRef coro_builder_emit_yield_from(
    CoroutineBuilder* cb,
    LLVMValueRef inner_coro_handle
);
```

### Phase 3: Finalization

```c
/**
 * Emit final suspend
 * - Emits coro.save + coro.suspend(final=true)
 * - Creates proper switch
 * - Connects to suspend block
 */
void coro_builder_emit_final_suspend(CoroutineBuilder* cb);

/**
 * Emit cleanup and suspend blocks
 * - Implements cleanup: coro.free + free
 * - Implements suspend: coro.end + ret
 * - Connects all paths properly
 */
void coro_builder_emit_cleanup_and_suspend(CoroutineBuilder* cb);

/**
 * Finalize and return the coroutine function
 * - Restores builder position
 * - Returns the completed function
 * - Destroys the builder
 */
LLVMValueRef coro_builder_finalize(CoroutineBuilder* cb);
```

### Helper Functions

```c
/**
 * Get the current insert block (for saving/restoring position)
 */
LLVMBasicBlockRef coro_builder_get_insert_block(CoroutineBuilder* cb);

/**
 * Position builder in a specific phase
 */
void coro_builder_position_at_start(CoroutineBuilder* cb);
void coro_builder_position_at_cleanup(CoroutineBuilder* cb);

/**
 * Create custom basic block within coroutine
 */
LLVMBasicBlockRef coro_builder_create_block(CoroutineBuilder* cb, const char* name);
```

---

## Usage Examples

### Example 1: Simple Coroutine (replaces ~150 lines with ~20)

```c
// Current: 150+ lines in compile_coroutine
// New:
LLVMValueRef compile_simple_coroutine(LLVMTypeRef yield_type, ...) {
  CoroutineBuilder* cb = coro_builder_create(module, builder, "my_coro",
                                              yield_type, NULL, 0);

  coro_builder_emit_initial_suspend(cb);

  // Body: yield 1; yield 2; yield 3;
  coro_builder_emit_yield(cb, LLVMConstInt(LLVMInt32Type(), 1, 0));
  coro_builder_emit_yield(cb, LLVMConstInt(LLVMInt32Type(), 2, 0));
  coro_builder_emit_yield(cb, LLVMConstInt(LLVMInt32Type(), 3, 0));

  coro_builder_emit_final_suspend(cb);
  coro_builder_emit_cleanup_and_suspend(cb);

  return coro_builder_finalize(cb);
}
```

### Example 2: cor_loop (replaces ~220 lines with ~40)

```c
LLVMValueRef CorLoopHandler(...) {
  LLVMTypeRef yield_type = extract_yield_type(coro_type);

  CoroutineBuilder* cb = coro_builder_create(module, builder, "coro_loop_wrapper",
                                              yield_type, NULL, 0);

  coro_builder_emit_initial_suspend(cb);

  // Infinite loop: evaluate inner coro, yield from it, repeat
  LLVMBasicBlockRef infinite_loop = coro_builder_create_block(cb, "infinite_loop");
  LLVMBuildBr(builder, infinite_loop);
  LLVMPositionBuilderAtEnd(builder, infinite_loop);

  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);
  coro_builder_emit_yield_from(cb, inner_handle);
  LLVMBuildBr(builder, infinite_loop); // Loop back

  // Note: This never reaches final suspend (infinite loop)
  // But we still need cleanup for destroy signal
  coro_builder_emit_cleanup_and_suspend(cb);

  return coro_builder_finalize(cb);
}
```

### Example 3: cor_map (replaces ~240 lines with ~50)

```c
LLVMValueRef CorMapHandler(...) {
  CoroutineBuilder* cb = coro_builder_create(module, builder, "coro_map_wrapper",
                                              output_type,
                                              (LLVMTypeRef[]){map_fn_type, GENERIC_PTR}, 2);

  coro_builder_emit_initial_suspend(cb);

  LLVMValueRef map_fn = LLVMGetParam(cb->function, 0);
  LLVMValueRef inner_handle = LLVMGetParam(cb->function, 1);

  // Loop through inner, apply map function, yield transformed values
  LLVMBasicBlockRef loop_start = coro_builder_create_block(cb, "map_loop");
  LLVMBuildBr(builder, loop_start);
  LLVMPositionBuilderAtEnd(builder, loop_start);

  // Check if inner done
  LLVMValueRef is_done = LLVMBuildCall2(..., get_coro_done_intrinsic(module), ...);
  LLVMBasicBlockRef done_bb = coro_builder_create_block(cb, "map_done");
  LLVMBasicBlockRef body_bb = coro_builder_create_block(cb, "map_body");
  LLVMBuildCondBr(builder, is_done, done_bb, body_bb);

  // Body: resume inner, read value, apply map, yield
  LLVMPositionBuilderAtEnd(builder, body_bb);
  LLVMBuildCall2(..., get_coro_resume_intrinsic(module), ...);
  LLVMValueRef inner_value = read_promise(inner_handle, ...);
  LLVMValueRef mapped = LLVMBuildCall2(builder, map_fn_type, map_fn,
                                        (LLVMValueRef[]){inner_value}, 1, "mapped");
  coro_builder_emit_yield(cb, mapped);
  LLVMBuildBr(builder, loop_start);

  // Done: final suspend
  LLVMPositionBuilderAtEnd(builder, done_bb);
  coro_builder_emit_final_suspend(cb);
  coro_builder_emit_cleanup_and_suspend(cb);

  return coro_builder_finalize(cb);
}
```

---

## Advanced Features for Future

### 1. Cancellation Support (for your Option 2)

```c
/**
 * Create a coroutine with cancellation support
 * - Extends promise to include cancellation flag
 * - Automatically checks flag at each yield point
 */
CoroutineBuilder* coro_builder_create_cancellable(
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    const char* name,
    LLVMTypeRef promise_type,
    LLVMTypeRef* param_types,
    int param_count
);

/**
 * Emit a yield with cancellation check
 * - Checks cancellation flag before storing to promise
 * - If cancelled, jumps to final suspend
 * - Otherwise, proceeds with normal yield
 */
LLVMBasicBlockRef coro_builder_emit_yield_cancellable(
    CoroutineBuilder* cb,
    LLVMValueRef value
);

/**
 * Get pointer to cancellation flag for external access
 */
LLVMValueRef coro_builder_get_cancel_flag_ptr(CoroutineBuilder* cb);
```

Usage:
```c
// In compile_coroutine:
CoroutineBuilder* cb = coro_builder_create_cancellable(module, builder, ...);

// Each yield automatically checks cancellation
coro_builder_emit_yield_cancellable(cb, value1);
coro_builder_emit_yield_cancellable(cb, value2);

// In CorStopHandler:
LLVMValueRef cancel_flag_ptr = coro_builder_get_cancel_flag_ptr_from_handle(handle);
LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 1, 0), cancel_flag_ptr);
```

### 2. Coroutine Combinators

```c
/**
 * High-level helpers for common patterns
 */

// Create a coroutine that yields from a list
LLVMValueRef coro_builder_create_list_iterator(
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    LLVMValueRef list_ptr,
    LLVMTypeRef element_type
);

// Create a coroutine that takes inner coro and transforms values
LLVMValueRef coro_builder_create_mapper(
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    LLVMValueRef map_fn,
    LLVMValueRef inner_coro
);

// Create infinite loop wrapper
LLVMValueRef coro_builder_create_loop_wrapper(
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    LLVMValueRef inner_coro
);
```

### 3. Debugging Support

```c
/**
 * Add debug information to yields
 */
void coro_builder_set_debug_info(CoroutineBuilder* cb, const char* source_file, int line);

/**
 * Name yields for easier LLVM IR reading
 */
void coro_builder_emit_yield_named(CoroutineBuilder* cb, LLVMValueRef value, const char* name);
```

---

## Implementation Strategy

### Phase 1: Core Builder (Week 1)
1. Implement `CoroutineBuilder` struct
2. Implement creation functions (create, emit_initial_suspend)
3. Implement basic yield (emit_yield)
4. Implement finalization (emit_final_suspend, emit_cleanup_and_suspend, finalize)
5. Test with simple coroutine case

### Phase 2: Migration (Week 2)
1. Refactor `compile_coroutine` to use builder
2. Refactor `CorLoopHandler` to use builder
3. Refactor `CorMapHandler` to use builder
4. Refactor `CorOfListHandler` to use builder
5. Verify all tests still pass

### Phase 3: Advanced Features (Week 3)
1. Implement `emit_yield_from` properly
2. Add cancellation support (create_cancellable, emit_yield_cancellable)
3. Implement helper combinators
4. Add debug support

### Phase 4: Optimization (Week 4)
1. Add IR-level optimizations within builder
2. Common subexpression elimination for repeated intrinsic calls
3. Dead block elimination
4. Constant folding for switches

---

## Benefits

### Code Reduction
- **compile_coroutine**: 220 lines → ~50 lines (77% reduction)
- **CorLoopHandler**: 220 lines → ~40 lines (82% reduction)
- **CorMapHandler**: 240 lines → ~50 lines (79% reduction)
- **CorOfListHandler**: 380 lines → ~60 lines (84% reduction)
- **Total**: ~1060 lines → ~200 lines (81% reduction)

### Maintainability
- Single source of truth for coroutine patterns
- Easier to add new features (cancellation, debugging)
- Easier to fix bugs (fix once, fixes everywhere)
- Less copy-paste errors

### Readability
- High-level intent is clear
- Coroutine structure is obvious
- Easy to understand control flow
- Self-documenting API

### Extensibility
- Easy to add new coroutine combinators
- Easy to add optimizations
- Easy to add debugging/profiling
- Easy to experiment with new patterns

---

## File Structure

```
lang/backend_llvm/coroutines/
├── coroutines.h              (existing)
├── coroutines.c              (refactored to use builder)
├── coroutine_builder.h       (NEW - builder API)
├── coroutine_builder.c       (NEW - builder implementation)
├── coroutine_extensions.h    (existing)
├── coroutine_extensions.c    (refactored to use builder)
└── coroutine_combinators.c   (NEW - high-level helpers)
```

---

## Alternative: Macro-Based Approach (Lighter Weight)

If a full builder is too heavy, we could use more extensive macros:

```c
#define CORO_BEGIN(name, yield_type) \
  CoroutineBuilder _cb = {0}; \
  coro_builder_init(&_cb, module, builder, name, yield_type); \
  coro_builder_emit_initial_suspend(&_cb);

#define CORO_YIELD(value) \
  coro_builder_emit_yield(&_cb, value);

#define CORO_YIELD_FROM(handle) \
  coro_builder_emit_yield_from(&_cb, handle);

#define CORO_END() \
  coro_builder_emit_final_suspend(&_cb); \
  coro_builder_emit_cleanup_and_suspend(&_cb); \
  return coro_builder_finalize(&_cb);

// Usage:
LLVMValueRef my_coro() {
  CORO_BEGIN("my_coro", LLVMInt32Type())
  CORO_YIELD(LLVMConstInt(LLVMInt32Type(), 1, 0))
  CORO_YIELD(LLVMConstInt(LLVMInt32Type(), 2, 0))
  CORO_END()
}
```

But this is less flexible and harder to debug.

---

## Questions for Discussion

1. **Scope**: Should we start with full builder or incremental macro approach?
2. **API Design**: Any preferences on function naming or structure?
3. **Cancellation**: Should cancellation be built-in from the start or added later?
4. **Testing**: How should we test the builder (unit tests, integration tests)?
5. **Migration**: Should we migrate all at once or one handler at a time?
6. **Performance**: Any concerns about abstraction overhead?

Let me know your thoughts and we can refine the design!
