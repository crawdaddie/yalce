#include "./coroutine_extensions.h"
#include "../../types/type_ser.h"
#include "../adt.h"
#include "../types.h"
#include "./coroutines/coroutines.h"
#include "llvm-c/Core.h"

// ============================================================================
// Coroutine Builtin Handlers (TODO: Implement these)
// ============================================================================
LLVMValueRef CorGetLastValHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorGetLastValHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Get the inner coroutine expression
  Ast *coro_ast = ast->data.AST_APPLICATION.args;
  Type *coro_type = coro_ast->type; // Type is Coroutine<T>

  // Extract yield type T from Coroutine<T>
  Type *yield_type = coro_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  // Create wrapper coroutine function
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(GENERIC_PTR, NULL, 0, 0);

  static int loop_counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_loop_wrapper_%d",
           loop_counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)

  // Create basic blocks
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_type =
      LLVMStructType((LLVMTypeRef[]){llvm_yield_type, LLVMInt1Type()}, 2, 0);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

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

  // Initial suspend
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

  // === LOOP BLOCK: Evaluate coroutine expression and drain it ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  // Evaluate the coroutine expression to get a handle
  // Each time through the loop, this creates a new coroutine instance
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // === YIELD-FROM LOOP (copied from codegen_yield) ===
  LLVMBasicBlockRef loop_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.check");
  LLVMBasicBlockRef loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.body");
  LLVMBasicBlockRef get_value_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.get_value");
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.resume");
  LLVMBasicBlockRef loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.exit");

  LLVMBuildBr(builder, loop_check_bb);

  // Check if inner is done
  LLVMPositionBuilderAtEnd(builder, loop_check_bb);
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
      "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // Resume inner
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
      get_coro_resume_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1, "");

  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle}, 1,
      "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, get_value_bb);

  // Get value from inner and yield it
  LLVMPositionBuilderAtEnd(builder, get_value_bb);

  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  LLVMValueRef inner_promise_ptr = LLVMBuildBitCast(
      builder, inner_promise_raw, LLVMPointerType(llvm_yield_type, 0),
      "inner.promise.ptr");

  LLVMValueRef inner_value = LLVMBuildLoad2(builder, llvm_yield_type,
                                            inner_promise_ptr, "inner.value");

  // Store in our promise
  LLVMBuildStore(builder, inner_value, promise_alloca);

  // Suspend
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "yield_from.suspend_return");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
  LLVMBuildBr(builder, loop_check_bb);

  // When inner is exhausted, go back to outer loop to create new instance
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);
  LLVMBuildBr(builder, loop_bb); // Infinite loop!

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);

  // Restore builder position
  LLVMPositionBuilderAtEnd(builder, prev_block);

  // Call the wrapper to create the looping coroutine handle
  LLVMValueRef loop_handle = LLVMBuildCall2(builder, wrapper_fn_type,
                                            wrapper_fn, NULL, 0, "loop.handle");

  return loop_handle;
}

LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  // Get the two arguments: map function and coroutine
  Ast *map_fn_ast = ast->data.AST_APPLICATION.args;
  Ast *coro_ast = ast->data.AST_APPLICATION.args + 1;

  // Type analysis: (T -> U) -> Coroutine<T> -> Coroutine<U>
  Type *map_fn_type = map_fn_ast->type;
  Type *input_type = map_fn_type->data.T_FN.from;  // T
  Type *output_type = fn_return_type(map_fn_type); // U

  Type *coro_type = coro_ast->type;                       // Coroutine<T>
  Type *coro_yield_type = coro_type->data.T_CONS.args[0]; // T

  LLVMTypeRef llvm_input_type = type_to_llvm_type(input_type, ctx, module);
  LLVMTypeRef llvm_output_type = type_to_llvm_type(output_type, ctx, module);

  // IMPORTANT: Evaluate arguments in CALLER's scope BEFORE creating coroutine
  LLVMValueRef map_fn = codegen(map_fn_ast, ctx, module, builder);
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES map function and coroutine as
  // parameters
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR, (LLVMTypeRef[]){LLVMTypeOf(map_fn), GENERIC_PTR}, 2, 0);

  static int counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_map_%d", counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Promise stores output type U
  LLVMTypeRef promise_type =
      LLVMStructType((LLVMTypeRef[]){llvm_output_type, LLVMInt1Type()}, 2, 0);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

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

  // Initial suspend
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

  // Get the parameters (map function and inner coroutine handle)
  LLVMValueRef map_fn_param = LLVMGetParam(wrapper_fn, 0);
  LLVMSetValueName(map_fn_param, "map_fn.param");
  LLVMValueRef inner_handle_param = LLVMGetParam(wrapper_fn, 1);
  LLVMSetValueName(inner_handle_param, "inner_handle.param");

  // Store map function in frame for repeated use
  LLVMValueRef map_fn_alloca =
      LLVMBuildAlloca(builder, LLVMTypeOf(map_fn_param), "map_fn");
  LLVMBuildStore(builder, map_fn_param, map_fn_alloca);

  // === YIELD-FROM LOOP with mapping ===
  LLVMBasicBlockRef loop_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "map.check");
  LLVMBasicBlockRef loop_body_bb = LLVMAppendBasicBlock(wrapper_fn, "map.body");
  LLVMBasicBlockRef get_value_bb =
      LLVMAppendBasicBlock(wrapper_fn, "map.get_value");
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "map.resume");
  LLVMBasicBlockRef loop_exit_bb = LLVMAppendBasicBlock(wrapper_fn, "map.exit");

  LLVMBuildBr(builder, loop_check_bb);

  // Check if inner is done
  LLVMPositionBuilderAtEnd(builder, loop_check_bb);
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_param}, 1,
      "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // Resume inner
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_param}, 1, "");

  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_param}, 1,
      "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, get_value_bb);

  // Get value from inner, apply map function, and yield
  LLVMPositionBuilderAtEnd(builder, get_value_bb);

  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle_param, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  LLVMValueRef inner_promise_ptr = LLVMBuildBitCast(
      builder, inner_promise_raw, LLVMPointerType(llvm_input_type, 0),
      "inner.promise.ptr");

  LLVMValueRef inner_value = LLVMBuildLoad2(builder, llvm_input_type,
                                            inner_promise_ptr, "inner.value");

  // Load and call map function
  LLVMValueRef loaded_map_fn = LLVMBuildLoad2(builder, LLVMTypeOf(map_fn_param),
                                              map_fn_alloca, "map_fn");

  LLVMTypeRef map_fn_llvm_type = LLVMFunctionType(
      llvm_output_type, (LLVMTypeRef[]){llvm_input_type}, 1, 0);

  LLVMValueRef mapped_value =
      LLVMBuildCall2(builder, map_fn_llvm_type, loaded_map_fn,
                     (LLVMValueRef[]){inner_value}, 1, "mapped.value");

  // Store mapped result in our promise
  LLVMBuildStore(builder, mapped_value, promise_alloca);

  // Suspend
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "map.suspend_return");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
  LLVMBuildBr(builder, loop_check_bb);

  // When inner is exhausted, finish
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

  // Final suspend
  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
      "final.suspend");

  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // Call the wrapper function, passing map function and inner coroutine as
  // arguments
  LLVMValueRef map_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){map_fn, inner_handle}, 2, "map.handle");

  return map_handle;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef handle_raw =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  LLVMBasicBlockRef check_resume_bb =
      LLVMAppendBasicBlock(current_fn, "check_resume");
  LLVMBasicBlockRef set_flag_bb =
      LLVMAppendBasicBlock(current_fn, "set_done_flag");
  LLVMBasicBlockRef done_bb = LLVMAppendBasicBlock(current_fn, "cor_stop_done");

  // Check if the handle is an integer or pointer type
  LLVMTypeRef handle_type = LLVMTypeOf(handle_raw);
  LLVMTypeKind type_kind = LLVMGetTypeKind(handle_type);

  LLVMValueRef handle;
  LLVMValueRef is_null_or_zero;

  if (type_kind == LLVMIntegerTypeKind) {
    // Handle is an integer (e.g., i64 from Uint64 0)
    // Check if it's 0
    is_null_or_zero = LLVMBuildICmp(builder, LLVMIntEQ, handle_raw,
                                    LLVMConstInt(handle_type, 0, 0), "is_zero");
    // Cast to pointer for further use
    handle = LLVMBuildIntToPtr(builder, handle_raw, GENERIC_PTR, "handle_ptr");
  } else {
    // Handle is already a pointer type
    handle = handle_raw;
    is_null_or_zero = LLVMBuildIsNull(builder, handle, "handle_is_null");
  }

  // If handle is null/zero, skip to done
  LLVMBuildCondBr(builder, is_null_or_zero, done_bb, check_resume_bb);

  // Check if resume function pointer is null
  LLVMPositionBuilderAtEnd(builder, check_resume_bb);
  LLVMValueRef resume_fn_ptr =
      LLVMBuildLoad2(builder, GENERIC_PTR, handle, "resume_fn");
  LLVMValueRef resume_is_null =
      LLVMBuildIsNull(builder, resume_fn_ptr, "resume_is_null");

  // If resume is null, skip to done, otherwise set the flag
  LLVMBuildCondBr(builder, resume_is_null, done_bb, set_flag_bb);

  // Set the is_done flag
  LLVMPositionBuilderAtEnd(builder, set_flag_bb);
  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(handle);
  Type *cor_type = ast->type;
  Type *yield_type = cor_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  LLVMTypeRef full_prom_type =
      LLVMStructType((LLVMTypeRef[]){llvm_yield_type, LLVMInt1Type()}, 2, 0);

  LLVMValueRef full_prom_ptr = LLVMBuildBitCast(
      builder, promise_ptr_raw, LLVMPointerType(full_prom_type, 0), "");

  LLVMValueRef is_done_flag_ptr = LLVMBuildStructGEP2(
      builder, full_prom_type, full_prom_ptr, 1, "get_is_done_flag");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 1, 0), is_done_flag_ptr);

  LLVMBuildBr(builder, done_bb);

  // Done
  LLVMPositionBuilderAtEnd(builder, done_bb);

  // LLVMValueRef coro_destroy = get_coro_destroy_intrinsic(module);
  //
  // LLVMBuildCall2(builder, LLVMGlobalGetValueType(coro_destroy), coro_destroy,
  //                (LLVMValueRef[]){handle}, 1, "");

  // cor_stop returns unit/void
  return LLVMGetUndef(LLVMVoidType());
}

LLVMValueRef CorOfCorListHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  // Get the list expression
  Ast *list_ast = ast->data.AST_APPLICATION.args;
  Type *list_type = list_ast->type; // Type is List<Coroutine<T>>

  // Extract element type Coroutine<T> from List<Coroutine<T>>
  Type *coro_type = list_type->data.T_CONS.args[0];

  // Extract yield type T from Coroutine<T>
  Type *elem_type = coro_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_elem_type = type_to_llvm_type(elem_type, ctx, module);
  LLVMTypeRef llvm_coro_type = GENERIC_PTR; // Coroutine handles are i8*
  LLVMTypeRef llvm_list_type = type_to_llvm_type(list_type, ctx, module);

  // IMPORTANT: Evaluate list expression in CALLER's scope BEFORE creating
  // coroutine
  LLVMValueRef list_ptr = codegen(list_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES the list as a parameter
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){llvm_list_type}, 1, 0);

  static int counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_of_cor_list_%d", counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  // Outer loop blocks - iterate through list
  LLVMBasicBlockRef outer_loop_bb = LLVMAppendBasicBlock(wrapper_fn, "outer.loop");
  LLVMBasicBlockRef outer_loop_body_bb = LLVMAppendBasicBlock(wrapper_fn, "outer.loop.body");
  LLVMBasicBlockRef outer_loop_exit_bb = LLVMAppendBasicBlock(wrapper_fn, "outer.loop.exit");

  // Inner loop blocks - yield from current coroutine
  LLVMBasicBlockRef inner_check_bb = LLVMAppendBasicBlock(wrapper_fn, "inner.check");
  LLVMBasicBlockRef inner_body_bb = LLVMAppendBasicBlock(wrapper_fn, "inner.body");
  LLVMBasicBlockRef inner_get_value_bb = LLVMAppendBasicBlock(wrapper_fn, "inner.get_value");
  LLVMBasicBlockRef inner_resume_bb = LLVMAppendBasicBlock(wrapper_fn, "inner.resume");
  LLVMBasicBlockRef inner_exit_bb = LLVMAppendBasicBlock(wrapper_fn, "inner.exit");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Promise holds the yielded element type T
  LLVMTypeRef promise_type = llvm_elem_type;
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Get the actual node type {Coroutine<T>, void*}
  LLVMTypeRef node_type = LLVMStructType(
      (LLVMTypeRef[]){llvm_coro_type, LLVMPointerType(LLVMVoidType(), 0)}, 2, 0);

  // Current list node pointer - llvm_list_type is already ptr to node
  LLVMValueRef current_alloca =
      LLVMBuildAlloca(builder, llvm_list_type, "current");

  // Current inner coroutine handle
  LLVMValueRef inner_handle_alloca =
      LLVMBuildAlloca(builder, llvm_coro_type, "inner.handle");

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

  // Get the list parameter (first argument to the wrapper function)
  LLVMValueRef list_param = LLVMGetParam(wrapper_fn, 0);
  LLVMSetValueName(list_param, "list.param");

  // Initialize current with the list parameter
  LLVMBuildStore(builder, list_param, current_alloca);
  LLVMBuildBr(builder, outer_loop_bb);

  // === OUTER LOOP: Iterate through list ===
  LLVMPositionBuilderAtEnd(builder, outer_loop_bb);

  // Load current pointer (ptr to node)
  LLVMValueRef current =
      LLVMBuildLoad2(builder, llvm_list_type, current_alloca, "current");

  // Check if current is NULL
  LLVMValueRef is_null = LLVMBuildIsNull(builder, current, "is.null");
  LLVMBuildCondBr(builder, is_null, outer_loop_exit_bb, outer_loop_body_bb);

  // === OUTER LOOP BODY: Get coroutine from list node ===
  LLVMPositionBuilderAtEnd(builder, outer_loop_body_bb);

  // GEP to data field (field 0) and load - this is the coroutine handle
  LLVMValueRef data_ptr =
      LLVMBuildStructGEP2(builder, node_type, current, 0, "data.ptr");
  LLVMValueRef inner_handle =
      LLVMBuildLoad2(builder, llvm_coro_type, data_ptr, "inner.handle");
  LLVMBuildStore(builder, inner_handle, inner_handle_alloca);

  // Now yield from this inner coroutine
  LLVMBuildBr(builder, inner_check_bb);

  // === INNER LOOP CHECK: Check if inner coroutine is done ===
  LLVMPositionBuilderAtEnd(builder, inner_check_bb);

  LLVMValueRef inner_handle_loaded =
      LLVMBuildLoad2(builder, llvm_coro_type, inner_handle_alloca, "inner.handle.loaded");

  // Check if inner is done BEFORE resuming
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_loaded}, 1,
      "inner.is_done_before");

  LLVMBuildCondBr(builder, is_done_before, inner_exit_bb, inner_body_bb);

  // === INNER LOOP BODY: Resume inner coroutine ===
  LLVMPositionBuilderAtEnd(builder, inner_body_bb);

  LLVMValueRef inner_handle_for_resume =
      LLVMBuildLoad2(builder, llvm_coro_type, inner_handle_alloca, "inner.handle.for_resume");

  // Resume the inner coroutine
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_for_resume}, 1, "");

  // Check if done AFTER resume
  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_for_resume}, 1,
      "inner.is_done_after");

  // If done after resume, exit the inner loop (move to next list element)
  LLVMBuildCondBr(builder, is_done_after, inner_exit_bb, inner_get_value_bb);

  // === INNER GET VALUE: Read inner's promise and yield it ===
  LLVMPositionBuilderAtEnd(builder, inner_get_value_bb);

  LLVMValueRef inner_handle_for_promise =
      LLVMBuildLoad2(builder, llvm_coro_type, inner_handle_alloca, "inner.handle.for_promise");

  // Get inner coroutine's promise
  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle_for_promise, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  // Cast to correct type
  LLVMValueRef inner_promise_ptr = LLVMBuildBitCast(
      builder, inner_promise_raw, LLVMPointerType(llvm_elem_type, 0),
      "inner.promise.ptr");

  // Load the value from inner's promise
  LLVMValueRef inner_value = LLVMBuildLoad2(builder, llvm_elem_type,
                                            inner_promise_ptr, "inner.value");

  // Store it in OUR promise (we're yielding this value)
  LLVMBuildStore(builder, inner_value, promise_alloca);

  // Suspend (yield this value to our caller)
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module),
      (LLVMValueRef[]){handle}, 1, "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "suspend.return");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), inner_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === INNER RESUME: When we're resumed, loop back to check for more values ===
  LLVMPositionBuilderAtEnd(builder, inner_resume_bb);
  LLVMBuildBr(builder, inner_check_bb); // Loop back to inner check!

  // === INNER EXIT: Current coroutine exhausted, move to next list element ===
  LLVMPositionBuilderAtEnd(builder, inner_exit_bb);

  // Load the current list node
  LLVMValueRef current_for_next =
      LLVMBuildLoad2(builder, llvm_list_type, current_alloca, "current.for_next");

  // GEP to next field (field 1) and load
  LLVMValueRef next_ptr_ptr =
      LLVMBuildStructGEP2(builder, node_type, current_for_next, 1, "next.ptr.ptr");
  LLVMValueRef next_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMVoidType(), 0), next_ptr_ptr, "next.ptr");

  // Cast void* to proper list pointer type (ptr to node)
  LLVMValueRef next_typed =
      LLVMBuildBitCast(builder, next_ptr, llvm_list_type, "next.typed");
  LLVMBuildStore(builder, next_typed, current_alloca);

  LLVMBuildBr(builder, outer_loop_bb); // Back to outer loop to get next coroutine

  // === OUTER LOOP EXIT: All list elements exhausted ===
  LLVMPositionBuilderAtEnd(builder, outer_loop_exit_bb);

  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
      "final.suspend");

  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // Call the wrapper function, passing the list as an argument
  LLVMValueRef coro_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){list_ptr}, 1, "cor_list.coro.handle");

  return coro_handle;
}

LLVMValueRef CorOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  // Get the list expression
  Ast *list_ast = ast->data.AST_APPLICATION.args;
  Type *list_type = list_ast->type; // Type is List<T>

  // Extract element type T from List<T>
  Type *elem_type = list_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_elem_type = type_to_llvm_type(elem_type, ctx, module);
  LLVMTypeRef llvm_list_type = type_to_llvm_type(list_type, ctx, module);

  // IMPORTANT: Evaluate list expression in CALLER's scope BEFORE creating
  // coroutine
  LLVMValueRef list_ptr = codegen(list_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES the list as a parameter
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){llvm_list_type}, 1, 0);

  static int counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_of_list_%d", counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "loop.body");
  LLVMBasicBlockRef loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "loop.exit");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_type =
      LLVMStructType((LLVMTypeRef[]){llvm_elem_type, LLVMInt1Type()}, 2, 0);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Get the actual node type {T, void*}
  LLVMTypeRef node_type = LLVMStructType(
      (LLVMTypeRef[]){llvm_elem_type, LLVMPointerType(LLVMVoidType(), 0)}, 2,
      0);

  // Current list node pointer - llvm_list_type is already ptr to node
  LLVMValueRef current_alloca =
      LLVMBuildAlloca(builder, llvm_list_type, "current");

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

  // Get the list parameter (first argument to the wrapper function)
  LLVMValueRef list_param = LLVMGetParam(wrapper_fn, 0);
  LLVMSetValueName(list_param, "list.param");

  // Initialize current with the list parameter
  LLVMBuildStore(builder, list_param, current_alloca);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  // Load current pointer (ptr to node)
  LLVMValueRef current =
      LLVMBuildLoad2(builder, llvm_list_type, current_alloca, "current");

  // Check if current is NULL
  LLVMValueRef is_null = LLVMBuildIsNull(builder, current, "is.null");
  LLVMBuildCondBr(builder, is_null, loop_exit_bb, loop_body_bb);

  // === LOOP BODY ===
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);

  // GEP to data field (field 0) and load
  LLVMValueRef data_ptr =
      LLVMBuildStructGEP2(builder, node_type, current, 0, "data.ptr");
  LLVMValueRef data =
      LLVMBuildLoad2(builder, llvm_elem_type, data_ptr, "node.data");
  LLVMBuildStore(builder, data, promise_alloca);

  // GEP to next field (field 1) and load
  LLVMValueRef next_ptr_ptr =
      LLVMBuildStructGEP2(builder, node_type, current, 1, "next.ptr.ptr");
  LLVMValueRef next_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMVoidType(), 0), next_ptr_ptr, "next.ptr");

  // Cast void* to proper list pointer type (ptr to node)
  LLVMValueRef next_typed =
      LLVMBuildBitCast(builder, next_ptr, llvm_list_type, "next.typed");
  LLVMBuildStore(builder, next_typed, current_alloca);

  // Suspend
  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "suspend.return");
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(wrapper_fn, "resume");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, resume_bb);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP EXIT ===
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
      "final.suspend");

  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // Call the wrapper function, passing the list as an argument
  LLVMValueRef coro_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){list_ptr}, 1, "list.coro.handle");

  return coro_handle;
}

LLVMValueRef CorOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  // Get the array expression
  Ast *array_ast = ast->data.AST_APPLICATION.args;
  Type *array_type = array_ast->type; // Type is Array<T>

  // Extract element type T from Array<T>
  Type *elem_type = array_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_elem_type = type_to_llvm_type(elem_type, ctx, module);
  LLVMTypeRef llvm_array_type = type_to_llvm_type(array_type, ctx, module);

  // IMPORTANT: Evaluate array expression in CALLER's scope BEFORE creating
  // coroutine
  LLVMValueRef array_val = codegen(array_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES the array as a parameter
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){llvm_array_type}, 1, 0);

  static int counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_of_array_%d", counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "loop.body");
  LLVMBasicBlockRef loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "loop.exit");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_struct_type =
      LLVMStructType((LLVMTypeRef[]){llvm_elem_type, LLVMInt1Type()}, 2, 0);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_struct_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_struct_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  LLVMValueRef array_alloca =
      LLVMBuildAlloca(builder, llvm_array_type, "array.alloca");

  LLVMValueRef counter_alloca =
      LLVMBuildAlloca(builder, LLVMInt32Type(), "counter");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), counter_alloca);

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

  // Get the array parameter (first argument to the wrapper function)
  LLVMValueRef array_param = LLVMGetParam(wrapper_fn, 0);
  LLVMSetValueName(array_param, "array.param");

  // Store the array parameter in the frame
  LLVMBuildStore(builder, array_param, array_alloca);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  LLVMValueRef counter_val =
      LLVMBuildLoad2(builder, LLVMInt32Type(), counter_alloca, "counter");
  LLVMValueRef array =
      LLVMBuildLoad2(builder, llvm_array_type, array_alloca, "array");

  // Array is {i32 size, ptr data}
  LLVMValueRef array_size =
      LLVMBuildExtractValue(builder, array, 0, "array.size");

  LLVMValueRef cmp =
      LLVMBuildICmp(builder, LLVMIntSLT, counter_val, array_size, "loop.cond");
  LLVMBuildCondBr(builder, cmp, loop_body_bb, loop_exit_bb);

  // === LOOP BODY ===
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);

  LLVMValueRef data_ptr =
      LLVMBuildExtractValue(builder, array, 1, "array.data");
  LLVMValueRef elem_ptr =
      LLVMBuildGEP2(builder, llvm_elem_type, data_ptr,
                    (LLVMValueRef[]){counter_val}, 1, "elem.ptr");
  LLVMValueRef elem = LLVMBuildLoad2(builder, llvm_elem_type, elem_ptr, "elem");

  LLVMBuildStore(builder, elem, promise_alloca);

  LLVMValueRef next_counter =
      LLVMBuildAdd(builder, counter_val, LLVMConstInt(LLVMInt32Type(), 1, 0),
                   "next.counter");
  LLVMBuildStore(builder, next_counter, counter_alloca);

  LLVMValueRef save_token = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "coro.save");

  LLVMValueRef suspend_result = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){save_token, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2,
      "coro.suspend");

  LLVMBasicBlockRef suspend_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "suspend.return");
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(wrapper_fn, "resume");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, resume_bb);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP EXIT ===
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

  LLVMValueRef final_save = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_save_intrinsic(module)),
      get_coro_save_intrinsic(module), (LLVMValueRef[]){handle}, 1,
      "final.save");

  LLVMValueRef final_suspend = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_suspend_intrinsic(module)),
      get_coro_suspend_intrinsic(module),
      (LLVMValueRef[]){final_save, LLVMConstInt(LLVMInt1Type(), 1, 0)}, 2,
      "final.suspend");

  LLVMBasicBlockRef final_return_bb =
      LLVMAppendBasicBlock(wrapper_fn, "final.return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === CLEANUP BLOCK ===
  LLVMPositionBuilderAtEnd(builder, cleanup_bb);

  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");

  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  // === SUSPEND BLOCK ===
  LLVMPositionBuilderAtEnd(builder, suspend_bb);

  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");

  LLVMBuildRet(builder, handle);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // Call the wrapper function, passing the array as an argument
  LLVMValueRef coro_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){array_val}, 1, "array.coro.handle");

  return coro_handle;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  Ast *time_ast = ast->data.AST_APPLICATION.args;
  Ast *schedule_event_ast = ast->data.AST_APPLICATION.args + 1;
  Ast *cor_ast = ast->data.AST_APPLICATION.args + 2;

  LLVMValueRef outer_handle = codegen(cor_ast, ctx, module, builder);
  if (!outer_handle) {
    return NULL;
  }

  LLVMValueRef schedule_event =
      codegen(schedule_event_ast, ctx, module, builder);

  if (!schedule_event) {
    return NULL;
  }

  LLVMValueRef u64ts = codegen(time_ast, ctx, module, builder);

  if (!u64ts) {
    return NULL;
  }

  LLVMValueRef func = LLVMAddFunction(
      module, "schedule_event_wrapper",
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){GENERIC_PTR, LLVMInt64Type()}, 2, 0));

  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMTypeRef schedule_event_type =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){LLVMInt64Type(), LLVMDoubleType(),
                                       GENERIC_PTR, GENERIC_PTR},
                       4, 0);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef finished =
      LLVMAppendBasicBlock(func, "coro.is_finished_block");
  LLVMBasicBlockRef not_finished =
      LLVMAppendBasicBlock(func, "coro.resume_block");

  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef _handle = LLVMGetParam(func, 0);
  LLVMValueRef _u64ts = LLVMGetParam(func, 1);
  LLVMTypeRef yield_type = LLVMDoubleType();

  LLVMValueRef resume_result =
      codegen_handle_resume(_handle, yield_type, ctx, module, builder);

  LLVMValueRef result_tag =
      LLVMBuildExtractValue(builder, resume_result, 0, "tag");

  LLVMValueRef is_done =
      LLVMBuildICmp(builder, LLVMIntEQ, result_tag,
                    LLVMConstInt(LLVMInt8Type(), 1, 0), "tag_eq_1");

  LLVMBuildCondBr(builder, is_done, finished, not_finished);

  // coroutine not finished - take yielded double and schedule next to happen at
  // u64ts + yielded
  LLVMPositionBuilderAtEnd(builder, not_finished);
  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(_handle);
  LLVMValueRef yield_ptr = LLVMBuildBitCast(
      builder, promise_ptr_raw, LLVMPointerType(yield_type, 0), "promise.ptr");

  // Load the yielded value
  LLVMValueRef yielded_value =
      LLVMBuildLoad2(builder, yield_type, yield_ptr, "yielded");

  LLVMValueRef scheduler_call =
      LLVMBuildCall2(builder, schedule_event_type, schedule_event,
                     (LLVMValueRef[]){
                         _u64ts,
                         yielded_value,
                         func,
                         _handle,
                     },
                     4, "schedule_next");
  LLVMBuildRetVoid(builder);

  // coroutine finished - don't continue to schedule and you can ignore the
  // yielded val
  LLVMPositionBuilderAtEnd(builder, finished);
  LLVMBuildRetVoid(builder);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  // LLVMDumpValue(func);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  LLVMBuildCall2(builder, schedule_event_type, schedule_event,
                 (LLVMValueRef[]){
                     u64ts,
                     LLVMConstReal(LLVMDoubleType(), 0.),
                     func,
                     outer_handle,
                 },
                 4, "call.schedule_event.now");
  return outer_handle;
}
// static LLVMValueRef __build_scheduled_cor_wrapper(LLVMTypeRef promise_type,
//                                                   LLVMValueRef scheduler,
//                                                   LLVMTypeRef scheduler_type,
//                                                   LLVMModuleRef module,
//                                                   LLVMBuilderRef builder) {
// LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

// LLVMTypeRef wrapper_fn_type =
//     LLVMFunctionType(LLVMVoidType(),
//                      (LLVMTypeRef[]){
//                          // LLVMPointerType(coro_obj_type, 0),
//                          LLVMInt64Type(),
//                      },
//                      2, 0);
// LLVMValueRef func =
//     LLVMAddFunction(module, "scheduler_wrapper", wrapper_fn_type);

// LLVMSetLinkage(func, LLVMExternalLinkage);
// LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
//
// LLVMBasicBlockRef finished =
//     LLVMAppendBasicBlock(func, "coro.is_finished_block");
// LLVMBasicBlockRef not_finished =
//     LLVMAppendBasicBlock(func, "coro.resume_block");
//
// LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
// LLVMPositionBuilderAtEnd(builder, entry);
// LLVMValueRef coro = LLVMGetParam(func, 0);
// LLVMValueRef timestamp = LLVMGetParam(func, 1);
// LLVMTypeRef val_type = LLVMDoubleType();
// CoroutineCtx coro_ctx = {.coro_obj_type = coro_obj_type,
//                          .promise_type = promise_type};
//
// coro = coro_advance(coro, &coro_ctx, builder);
//
// LLVMValueRef is_finished = coro_is_finished(coro, &coro_ctx, builder);
// LLVMBuildCondBr(builder, is_finished, finished, not_finished);
// LLVMPositionBuilderAtEnd(builder, not_finished);
// LLVMValueRef promise =
//     coro_promise(coro, coro_obj_type, promise_type, builder);
// LLVMValueRef val = LLVMBuildExtractValue(builder, promise, 1, "");
//

//
// LLVMPositionBuilderAtEnd(builder, finished);
// LLVMBuildRetVoid(builder);
//
// LLVMPositionBuilderAtEnd(builder, prev_block);
// return func;
// }

LLVMValueRef CurrentCorHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CurrentCorHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorUnwrapOrEndHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorUnwrapOrEndHandler not yet implemented\n");
  return NULL;
}
