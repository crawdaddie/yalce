# Coroutine Helpers Usage Example

## Example: Refactoring cor_loop Handler

### Before (220 lines with all boilerplate)

```c
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  Ast *coro_ast = ast->data.AST_APPLICATION.args;
  Type *coro_type = coro_ast->type;
  Type *yield_type = coro_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
  static int loop_counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_loop_wrapper_%d",
           loop_counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK === (15 lines of boilerplate)
  LLVMPositionBuilderAtEnd(builder, entry_bb);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, llvm_yield_type, "promise");

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  LLVMValueRef frame =
      LLVMBuildArrayMalloc(builder, LLVMInt8Type(), size, "coro.frame");

  LLVMValueRef handle = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_begin_intrinsic(module)),
      get_coro_begin_intrinsic(module), (LLVMValueRef[]){id, frame}, 2,
      "coro.handle");

  // Initial suspend (15 lines of boilerplate)
  LLVMValueRef initial_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "initial.save");

  LLVMValueRef initial_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){initial_save, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "initial.suspend");

  LLVMValueRef init_switch =
      LLVMBuildSwitch(builder, initial_suspend, initial_return_bb, 2);
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 0, 0), start_bb);
  LLVMAddCase(init_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, initial_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === START BLOCK ===
  LLVMPositionBuilderAtEnd(builder, start_bb);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // ... 80+ lines of yield-from loop boilerplate ...
  // (check if done, resume, read promise, yield, suspend, switch, etc.)

  // === CLEANUP === (10+ lines)
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND === (5+ lines)
  LLVMPositionBuilderAtEnd(builder, suspend_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return wrapper_fn;
}
```

### After (50 lines with helpers)

```c
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Extract types (same as before)
  Ast *coro_ast = ast->data.AST_APPLICATION.args;
  Type *coro_type = coro_ast->type;
  Type *yield_type = coro_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  // Create wrapper function (same as before)
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);
  static int loop_counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_loop_wrapper_%d",
           loop_counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK === (1 helper call replaces 15 lines)
  LLVMPositionBuilderAtEnd(builder, entry_bb);
  CoroSetupResult setup = coro_emit_setup(ctx, module, builder, llvm_yield_type);

  // === INITIAL SUSPEND === (1 helper call replaces 15 lines)
  coro_emit_initial_suspend(ctx, module, builder, setup.handle, cleanup_bb,
                            suspend_bb, initial_return_bb, start_bb);
  // Now positioned at start_bb automatically

  // === INFINITE LOOP ===
  LLVMBuildBr(builder, loop_bb);
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  // Evaluate inner coroutine
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // === YIELD-FROM LOOP === (1 helper call replaces 80+ lines)
  LLVMBasicBlockRef loop_exit = coro_emit_yield_from_loop(
      ctx, module, builder, setup.handle, inner_handle, setup.promise_alloca,
      llvm_yield_type, cleanup_bb, suspend_bb, "loop");

  // Loop forever - jump back to loop_bb
  LLVMBuildBr(builder, loop_bb);

  // === CLEANUP & SUSPEND === (1 helper call replaces 15+ lines)
  coro_emit_cleanup_and_suspend(ctx, module, builder, setup.coro_id,
                                setup.handle, cleanup_bb, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return wrapper_fn;
}
```

**Reduction: 220 lines → 50 lines (77% reduction!)**

---

## Example: Using coro_emit_yield in compile_coroutine

### Before (25-30 lines per yield)

```c
// In compile_coroutine's codegen_yield function
LLVMBuildStore(builder, yield_value, coro_ctx->promise_alloca);

LLVMValueRef save_token = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
    get_coro_save_intrinsic(module), (LLVMValueRef[]){coro_ctx->coro_handle},
    1, "coro.save");

LLVMValueRef suspend_result = LLVMBuildCall2(
    builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
    get_coro_suspend_intrinsic(module),
    (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
    "coro.suspend");

LLVMBasicBlockRef return_bb = LLVMAppendBasicBlock(..., "yield.return");
LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(..., "yield.resume");

LLVMValueRef switch_inst =
    LLVMBuildSwitch(builder, suspend_result, return_bb, 2);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0),
            coro_ctx->cleanup_bb);

LLVMPositionBuilderAtEnd(builder, return_bb);
LLVMBuildBr(builder, coro_ctx->suspend_bb);

LLVMPositionBuilderAtEnd(builder, resume_bb);

coro_ctx->yield_count++;
```

### After (1 line)

```c
// In compile_coroutine's codegen_yield function
coro_emit_yield(ctx, module, builder, coro_ctx, yield_value);
// Now positioned at resume block, ready for next statement
```

**Reduction: 25 lines → 1 line (96% reduction per yield!)**

---

## Key Benefits

1. **Massive code reduction**: 75-95% less boilerplate
2. **Works with existing context**: Uses `JITLangCtx` and `CoroutineCtx`
3. **Automatic positioning**: Helpers position builder correctly for next step
4. **Error reduction**: Less copy-paste means fewer bugs
5. **Easy to extend**: Add cancellation to `coro_emit_yield` in one place

## Integration Steps

1. ✅ Created `coroutine_helpers.h` with declarations
2. ✅ Created `coroutine_helpers.c` with implementations
3. ✅ Added include to `coroutines.h`
4. ✅ Makefile already picks up `*.c` files automatically
5. Next: Refactor existing handlers to use helpers
6. Next: Add cancellation support to helpers

## Testing Plan

1. Build with `make` - verify `coroutine_helpers.c` compiles
2. Keep existing handlers as-is initially
3. Create test coroutine using helpers
4. Compare IR output with old implementation
5. Once verified, incrementally refactor existing handlers
