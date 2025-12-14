#include "./coroutine_extensions.h"
#include "coroutines/coroutines.h"
#include "types.h"
#include "types/type_ser.h"
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

  // Create wrapper coroutine function that TAKES map function and coroutine as parameters
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMTypeOf(map_fn), GENERIC_PTR},
      2, 0);

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
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, llvm_output_type, "promise");

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
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
      get_coro_resume_intrinsic(module), (LLVMValueRef[]){inner_handle_param}, 1, "");

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
  LLVMValueRef loaded_map_fn =
      LLVMBuildLoad2(builder, LLVMTypeOf(map_fn_param), map_fn_alloca, "map_fn");

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

  // Call the wrapper function, passing map function and inner coroutine as arguments
  LLVMValueRef map_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){map_fn, inner_handle}, 2, "map.handle");

  return map_handle;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CorStopHandler not yet implemented\n");
  return NULL;
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

  // IMPORTANT: Evaluate list expression in CALLER's scope BEFORE creating coroutine
  LLVMValueRef list_ptr = codegen(list_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES the list as a parameter
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){llvm_list_type},
      1, 0);

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

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, llvm_elem_type, "promise");

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
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
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

  // IMPORTANT: Evaluate array expression in CALLER's scope BEFORE creating coroutine
  LLVMValueRef array_val = codegen(array_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES the array as a parameter
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){llvm_array_type},
      1, 0);

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

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, llvm_elem_type, "promise");
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
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){array_val}, 1, "array.coro.handle");

  return coro_handle;
}

LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: PlayRoutineHandler not yet implemented\n");
  return NULL;
}

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
