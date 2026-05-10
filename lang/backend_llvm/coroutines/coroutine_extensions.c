#include "./coroutine_extensions.h"
#include "../../types/type_ser.h"
#include "../adt.h"
#include "../closures.h"
#include "../function.h"
#include "../tuple.h"
#include "../types.h"
#include "./coroutines.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <string.h>

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

  // Promise type for the inner coroutine (to read reset_fn/args_ptr)
  LLVMTypeRef inner_prom_type = CORO_PROMISE_TYPE(llvm_yield_type);

  // Create wrapper coroutine function — takes just the inner handle
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, "coro_loop_wrapper", wrapper_fn_type);

  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)

  // Create basic blocks
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef loop_bb = LLVMAppendBasicBlock(wrapper_fn, "loop");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Wrapper's own promise type (just yield value + is_done)
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_yield_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Alloca to store the inner handle (it gets mutated by memcpy restore)
  LLVMValueRef inner_handle_slot =
      LLVMBuildAlloca(builder, GENERIC_PTR, "inner_handle.slot");

  // Store the inner handle param before initial suspend
  LLVMBuildStore(builder, LLVMGetParam(wrapper_fn, 0), inner_handle_slot);

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null in wrapper's promise
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

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

  // === START BLOCK === (just fall through to loop)
  LLVMPositionBuilderAtEnd(builder, start_bb);
  LLVMBuildBr(builder, loop_bb);

  // === LOOP BLOCK: Evaluate coroutine expression and drain it ===
  LLVMPositionBuilderAtEnd(builder, loop_bb);

  // Load inner handle from alloca
  LLVMValueRef inner_handle_loop =
      LLVMBuildLoad2(builder, GENERIC_PTR, inner_handle_slot, "inner_handle");

  // Read reset_fn and args_ptr from the INNER coroutine's promise
  LLVMValueRef inner_prom_ptr =
      GET_PROMISE_PTR(inner_handle_loop, inner_prom_type);
  LLVMValueRef reset_closure =
      PROMISE_GET_RESET_FN(inner_prom_ptr, inner_prom_type);
  LLVMValueRef args_ptr = PROMISE_GET_ARGS_PTR(inner_prom_ptr, inner_prom_type);

  // === YIELD-FROM LOOP ===
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
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_loop}, 1,
      "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // Resume inner
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_loop}, 1, "");

  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_loop}, 1,
      "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, get_value_bb);

  // Get value from inner and yield it
  LLVMPositionBuilderAtEnd(builder, get_value_bb);

  LLVMValueRef inner_value =
      PROMISE_GET_VALUE(inner_prom_ptr, inner_prom_type, llvm_yield_type);

  // Store in our promise (field 0)
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

  // When inner is exhausted, reset it and loop
  LLVMPositionBuilderAtEnd(builder, loop_exit_bb);

  // Alloca for frame size - placed here (not entry) so it stays on stack
  LLVMValueRef frame_size_slot =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.slot");

  // Reset closure signature: (ptr frame_size_out, ptr args_ptr) -> ptr
  // or for void args: (ptr frame_size_out) -> ptr
  // We use the 2-arg version and pass args_ptr (which may be null)
  LLVMTypeRef closure_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), GENERIC_PTR}, 2, 0);

  LLVMValueRef new_coro = LLVMBuildCall2(
      builder, closure_type, reset_closure,
      (LLVMValueRef[]){frame_size_slot, args_ptr}, 2, "call_reset_closure");

  LLVMValueRef _frame_size =
      LLVMBuildLoad2(builder, LLVMInt64Type(), frame_size_slot, "frame_size");

  coro_emit_memcpy_restore(inner_handle_loop, new_coro, _frame_size, builder);

  LLVMBuildBr(builder, loop_bb);

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

  // Codegen the inner coroutine — returns a handle (reset_fn/args_ptr are in
  // its promise)
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // Call the wrapper to create the looping coroutine handle
  LLVMValueRef loop_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){inner_handle}, 1, "loop.handle");

  return loop_handle;
}

LLVMValueRef coro_map_reset_fn(LLVMTypeRef promise_type,
                               LLVMTypeRef inner_prom_type,
                               LLVMTypeRef wrapper_fn_type,
                               LLVMValueRef wrapper_fn, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  // Create reset closure for mapped coroutine
  // Closure args: {map_fn_ptr, inner_handle}
  LLVMTypeRef closure_args_ty =
      LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);

  // Reset closure signature: (ptr frame_size_out, ptr args_ptr) -> ptr handle
  LLVMTypeRef reset_closure_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), GENERIC_PTR}, 2, 0);

  LLVMValueRef reset_closure_fn =
      LLVMAddFunction(module, "coro_map.reset", reset_closure_type);
  LLVMSetLinkage(reset_closure_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef reset_prev = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef reset_entry =
      LLVMAppendBasicBlock(reset_closure_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, reset_entry);

  // Reset closure params: (ptr frame_size_out, ptr args_ptr)
  LLVMValueRef reset_frame_size_out = LLVMGetParam(reset_closure_fn, 0);
  LLVMValueRef reset_args_raw = LLVMGetParam(reset_closure_fn, 1);

  // Load fields from closure args struct
  LLVMValueRef reset_args =
      LLVMBuildBitCast(builder, reset_args_raw,
                       LLVMPointerType(closure_args_ty, 0), "reset_args");

  LLVMValueRef r_map_fn = LLVMBuildLoad2(
      builder, GENERIC_PTR,
      LLVMBuildStructGEP2(builder, closure_args_ty, reset_args, 0, ""),
      "map_fn");
  LLVMValueRef r_inner_handle = LLVMBuildLoad2(
      builder, GENERIC_PTR,
      LLVMBuildStructGEP2(builder, closure_args_ty, reset_args, 1, ""),
      "inner_handle");

  // Get inner's reset_fn and args_ptr from its promise
  LLVMValueRef inner_prom_ptr =
      GET_PROMISE_PTR(r_inner_handle, inner_prom_type);
  LLVMValueRef r_inner_reset =
      PROMISE_GET_RESET_FN(inner_prom_ptr, inner_prom_type);
  LLVMValueRef r_inner_args =
      PROMISE_GET_ARGS_PTR(inner_prom_ptr, inner_prom_type);

  // Alloca for inner's frame size
  LLVMValueRef inner_frame_size_slot =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "inner_frame_size.ignored");

  // Call inner reset closure to get a fresh inner handle
  LLVMValueRef fresh_inner = LLVMBuildCall2(
      builder, reset_closure_type, r_inner_reset,
      (LLVMValueRef[]){inner_frame_size_slot, r_inner_args}, 2, "fresh_inner");

  // Call wrapper_fn(frame_size_out, map_fn, fresh_inner) to create new mapped
  // coroutine. The wrapper will write its frame size to frame_size_out.
  LLVMValueRef new_map_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){reset_frame_size_out, r_map_fn, fresh_inner}, 3,
      "new_map_handle");

  // Write reset_fn and args_ptr into the new handle's promise
  LLVMValueRef new_prom_ptr = GET_PROMISE_PTR(new_map_handle, promise_type);
  PROMISE_SET_RESET_FN(new_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(new_prom_ptr, promise_type, reset_args_raw);

  LLVMBuildRet(builder, new_map_handle);

  LLVMPositionBuilderAtEnd(builder, reset_prev);
  return reset_closure_fn;
}

bool is_compose_fn(Ast *ast) {
  return ast->tag == AST_APPLICATION &&
         ast->data.AST_APPLICATION.function->tag == AST_IDENTIFIER &&
         strcmp(ast->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
                "fn_composition") == 0;
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

  // Promise type for inner coroutine (to read reset_fn/args_ptr)
  LLVMTypeRef inner_prom_type = CORO_PROMISE_TYPE(llvm_input_type);
  //
  // LLVMValueRef map_fn = codegen(map_fn_ast, ctx, module, builder);
  LLVMValueRef map_fn;
  if (map_fn_ast->type->kind == T_FN && !is_compose_fn(map_fn_ast)) {
    map_fn = codegen_fn(map_fn_ast, ctx, module, builder);
  } else {
    map_fn = codegen(map_fn_ast, ctx, module, builder);
  }

  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES frame_size_out, map function,
  // and coroutine as parameters
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0),
                                       LLVMTypeOf(map_fn), GENERIC_PTR},
                       3, 0);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, "coro_map", wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Promise stores output type U with 4-field layout
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_output_type);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  // Write frame size to caller's out-param (param 0)
  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  // Get the parameters (param 0 is frame_size_out, param 1 is map fn, param 2
  // is inner handle)
  LLVMValueRef map_fn_param = LLVMGetParam(wrapper_fn, 1);
  LLVMSetValueName(map_fn_param, "map_fn.param");
  LLVMValueRef inner_handle_param = LLVMGetParam(wrapper_fn, 2);
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

  // Alloca for frame size (not used here, but wrapper expects it)
  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  // Call the wrapper function, passing frame_size_out, map function, and inner
  // coroutine
  LLVMValueRef map_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){frame_size_alloca, map_fn, inner_handle},
                     3, "map.handle");

  // Create reset closure for mapped coroutine
  // Closure args: {map_fn_ptr, inner_handle}
  LLVMTypeRef closure_args_ty =
      LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);

  LLVMValueRef reset_closure_fn =
      coro_map_reset_fn(promise_type, inner_prom_type, wrapper_fn_type,
                        wrapper_fn, module, builder);

  // Allocate and populate closure args struct
  LLVMValueRef args_struct = LLVMGetUndef(closure_args_ty);
  args_struct = LLVMBuildInsertValue(builder, args_struct, map_fn, 0, "");
  args_struct = LLVMBuildInsertValue(builder, args_struct, inner_handle, 1, "");

  LLVMValueRef args_ptr_alloc = LLVMBuildMalloc(builder, closure_args_ty, "");
  LLVMBuildStore(builder, args_struct, args_ptr_alloc);

  // Write reset_fn and args_ptr into map_handle's promise
  LLVMValueRef map_prom_ptr = GET_PROMISE_PTR(map_handle, promise_type);
  PROMISE_SET_RESET_FN(map_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(map_prom_ptr, promise_type, args_ptr_alloc);

  return map_handle;
}

LLVMValueRef CorFilterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  // Get the two arguments: filter predicate and coroutine
  Ast *filter_fn_ast = ast->data.AST_APPLICATION.args;
  Ast *coro_ast = ast->data.AST_APPLICATION.args + 1;

  // Type analysis: (T -> Bool) -> Coroutine<T> -> Coroutine<T>
  Type *filter_fn_type = filter_fn_ast->type;
  Type *elem_type = filter_fn_type->data.T_FN.from; // T

  LLVMTypeRef llvm_elem_type = type_to_llvm_type(elem_type, ctx, module);

  // Promise type for inner coroutine (to read reset_fn/args_ptr)
  LLVMTypeRef inner_prom_type = CORO_PROMISE_TYPE(llvm_elem_type);

  LLVMValueRef filter_fn = codegen(filter_fn_ast, ctx, module, builder);
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // Create wrapper coroutine function that TAKES frame_size_out, filter
  // function, and coroutine as parameters
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0),
                                       LLVMTypeOf(filter_fn), GENERIC_PTR},
                       3, 0);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, "coro_filter", wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Promise stores element type T with 4-field layout (filter preserves type)
  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_elem_type);

  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  // Write frame size to caller's out-param (param 0)
  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  // Get the parameters (param 0 is frame_size_out, param 1 is filter fn, param
  // 2 is inner handle)
  LLVMValueRef filter_fn_param = LLVMGetParam(wrapper_fn, 1);
  LLVMSetValueName(filter_fn_param, "filter_fn.param");
  LLVMValueRef inner_handle_param = LLVMGetParam(wrapper_fn, 2);
  LLVMSetValueName(inner_handle_param, "inner_handle.param");

  // Store filter function in frame for repeated use
  LLVMValueRef filter_fn_alloca =
      LLVMBuildAlloca(builder, LLVMTypeOf(filter_fn_param), "filter_fn");
  LLVMBuildStore(builder, filter_fn_param, filter_fn_alloca);

  // === FILTER LOOP: consume inner until predicate passes or exhausted ===
  LLVMBasicBlockRef loop_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "filter.check");
  LLVMBasicBlockRef loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "filter.body");
  LLVMBasicBlockRef test_predicate_bb =
      LLVMAppendBasicBlock(wrapper_fn, "filter.test");
  LLVMBasicBlockRef yield_bb = LLVMAppendBasicBlock(wrapper_fn, "filter.yield");
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "filter.resume");
  LLVMBasicBlockRef loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "filter.exit");

  LLVMBuildBr(builder, loop_check_bb);

  // Check if inner is done before resuming
  LLVMPositionBuilderAtEnd(builder, loop_check_bb);
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_param}, 1,
      "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // Resume inner to get next value
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_param}, 1, "");

  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_param}, 1,
      "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, test_predicate_bb);

  // Get value from inner and test predicate
  LLVMPositionBuilderAtEnd(builder, test_predicate_bb);

  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle_param, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  LLVMValueRef inner_promise_ptr =
      LLVMBuildBitCast(builder, inner_promise_raw,
                       LLVMPointerType(llvm_elem_type, 0), "inner.promise.ptr");

  LLVMValueRef inner_value =
      LLVMBuildLoad2(builder, llvm_elem_type, inner_promise_ptr, "inner.value");

  // Load and call filter predicate
  LLVMValueRef loaded_filter_fn = LLVMBuildLoad2(
      builder, LLVMTypeOf(filter_fn_param), filter_fn_alloca, "filter_fn");

  // Filter function returns Bool (i1)
  LLVMTypeRef filter_fn_llvm_type =
      LLVMFunctionType(LLVMInt1Type(), (LLVMTypeRef[]){llvm_elem_type}, 1, 0);

  LLVMValueRef predicate_result =
      LLVMBuildCall2(builder, filter_fn_llvm_type, loaded_filter_fn,
                     (LLVMValueRef[]){inner_value}, 1, "predicate.result");

  // If predicate passes, yield; otherwise loop back to consume more
  LLVMBuildCondBr(builder, predicate_result, yield_bb, loop_check_bb);

  // Yield the value that passed the predicate
  LLVMPositionBuilderAtEnd(builder, yield_bb);

  // Store the passing value in our promise (same type, no transformation)
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
      LLVMAppendBasicBlock(wrapper_fn, "filter.suspend_return");

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

  // Alloca for frame size (not used here, but wrapper expects it)
  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  // Call the wrapper function, passing frame_size_out, filter function, and
  // inner coroutine
  LLVMValueRef filter_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){frame_size_alloca, filter_fn, inner_handle}, 3,
      "filter.handle");

  // Create reset closure for filtered coroutine
  // Closure args: {filter_fn_ptr, inner_handle}
  LLVMTypeRef closure_args_ty =
      LLVMStructType((LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);

  // Reuse coro_map_reset_fn since filter has same closure structure
  LLVMValueRef reset_closure_fn =
      coro_map_reset_fn(promise_type, inner_prom_type, wrapper_fn_type,
                        wrapper_fn, module, builder);

  // Allocate and populate closure args struct
  LLVMValueRef args_struct = LLVMGetUndef(closure_args_ty);
  args_struct = LLVMBuildInsertValue(builder, args_struct, filter_fn, 0, "");
  args_struct = LLVMBuildInsertValue(builder, args_struct, inner_handle, 1, "");

  LLVMValueRef args_ptr_alloc = LLVMBuildMalloc(builder, closure_args_ty, "");
  LLVMBuildStore(builder, args_struct, args_ptr_alloc);

  // Write reset_fn and args_ptr into filter_handle's promise
  LLVMValueRef filter_prom_ptr = GET_PROMISE_PTR(filter_handle, promise_type);
  PROMISE_SET_RESET_FN(filter_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(filter_prom_ptr, promise_type, args_ptr_alloc);

  return filter_handle;
}

LLVMValueRef coro_take_reset_fn(LLVMTypeRef promise_type,
                                LLVMTypeRef inner_prom_type,
                                LLVMTypeRef wrapper_fn_type,
                                LLVMValueRef wrapper_fn, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  // Create reset closure for coro_take coroutine
  //
  // Closure args: {take_count (i32), inner_handle (ptr)}
  LLVMTypeRef closure_args_ty =
      LLVMStructType((LLVMTypeRef[]){LLVMInt32Type(), GENERIC_PTR}, 2, 0);

  // Reset closure signature: (ptr frame_size_out, ptr args_ptr) -> ptr handle
  LLVMTypeRef reset_closure_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), GENERIC_PTR}, 2, 0);

  LLVMValueRef reset_closure_fn =
      LLVMAddFunction(module, "coro_take.reset", reset_closure_type);
  LLVMSetLinkage(reset_closure_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef reset_prev = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef reset_entry =
      LLVMAppendBasicBlock(reset_closure_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, reset_entry);

  // Reset closure params: (ptr frame_size_out, ptr args_ptr)
  LLVMValueRef reset_frame_size_out = LLVMGetParam(reset_closure_fn, 0);
  LLVMValueRef reset_args_raw = LLVMGetParam(reset_closure_fn, 1);

  // Load fields from closure args struct
  LLVMValueRef reset_args =
      LLVMBuildBitCast(builder, reset_args_raw,
                       LLVMPointerType(closure_args_ty, 0), "reset_args");

  LLVMValueRef r_take_count = LLVMBuildLoad2(
      builder, LLVMInt32Type(),
      LLVMBuildStructGEP2(builder, closure_args_ty, reset_args, 0, ""),
      "take_count");
  LLVMValueRef r_inner_handle = LLVMBuildLoad2(
      builder, GENERIC_PTR,
      LLVMBuildStructGEP2(builder, closure_args_ty, reset_args, 1, ""),
      "inner_handle");

  // Get inner's reset_fn and args_ptr from its promise
  LLVMValueRef inner_prom_ptr =
      GET_PROMISE_PTR(r_inner_handle, inner_prom_type);
  LLVMValueRef r_inner_reset =
      PROMISE_GET_RESET_FN(inner_prom_ptr, inner_prom_type);
  LLVMValueRef r_inner_args =
      PROMISE_GET_ARGS_PTR(inner_prom_ptr, inner_prom_type);

  // Alloca for inner's frame size
  LLVMValueRef inner_frame_size_slot =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "inner_frame_size.ignored");

  // Call inner reset closure to get a fresh inner handle
  LLVMValueRef fresh_inner = LLVMBuildCall2(
      builder, reset_closure_type, r_inner_reset,
      (LLVMValueRef[]){inner_frame_size_slot, r_inner_args}, 2, "fresh_inner");

  // Call wrapper_fn(frame_size_out, count, fresh_inner)
  LLVMValueRef new_take_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){reset_frame_size_out, r_take_count, fresh_inner}, 3,
      "new_take_handle");

  // Write reset_fn and args_ptr into the new handle's promise
  LLVMValueRef new_prom_ptr = GET_PROMISE_PTR(new_take_handle, promise_type);
  PROMISE_SET_RESET_FN(new_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(new_prom_ptr, promise_type, reset_args_raw);

  LLVMBuildRet(builder, new_take_handle);

  LLVMPositionBuilderAtEnd(builder, reset_prev);
  return reset_closure_fn;
}

LLVMValueRef CorTakeHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Get the two arguments: count n and coroutine
  Ast *count_ast = ast->data.AST_APPLICATION.args;
  Ast *coro_ast = ast->data.AST_APPLICATION.args + 1;

  // Type analysis: Int -> Coroutine<T> -> Coroutine<T>
  Type *coro_type = coro_ast->type;
  Type *yield_type = coro_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);

  // Promise type for inner coroutine
  LLVMTypeRef inner_prom_type = CORO_PROMISE_TYPE(llvm_yield_type);

  LLVMValueRef count_val = codegen(count_ast, ctx, module, builder);
  LLVMValueRef inner_handle = codegen(coro_ast, ctx, module, builder);

  // Create wrapper coroutine function: (frame_size_out, count, inner_handle) ->
  // handle
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0),
                                       LLVMInt32Type(), GENERIC_PTR},
                       3, 0);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, "coro_take_wrapper", wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_yield_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Alloca for remaining counter
  LLVMValueRef remaining_slot =
      LLVMBuildAlloca(builder, LLVMInt32Type(), "remaining.slot");

  // Alloca to store the inner handle (must be saved before initial suspend)
  LLVMValueRef inner_handle_slot =
      LLVMBuildAlloca(builder, GENERIC_PTR, "inner_handle.slot");

  // Store initial count before initial suspend (param 1 is i32)
  LLVMValueRef count_param = LLVMGetParam(wrapper_fn, 1);
  LLVMBuildStore(builder, count_param, remaining_slot);

  // Store inner handle before initial suspend (param 2)
  LLVMBuildStore(builder, LLVMGetParam(wrapper_fn, 2), inner_handle_slot);

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null (will be set later)
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  // Write frame size to caller's out-param (param 0)
  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  // === TAKE LOOP ===
  LLVMBasicBlockRef loop_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.check");
  LLVMBasicBlockRef count_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.count_check");
  LLVMBasicBlockRef loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.body");
  LLVMBasicBlockRef get_value_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.get_value");
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.resume");
  LLVMBasicBlockRef loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "take.exit");

  LLVMBuildBr(builder, loop_check_bb);

  // First check: is remaining > 0?
  LLVMPositionBuilderAtEnd(builder, loop_check_bb);
  LLVMValueRef remaining =
      LLVMBuildLoad2(builder, LLVMInt32Type(), remaining_slot, "remaining");

  LLVMValueRef has_remaining =
      LLVMBuildICmp(builder, LLVMIntSGT, remaining,
                    LLVMConstInt(LLVMInt32Type(), 0, 0), "has_remaining");
  LLVMBuildCondBr(builder, has_remaining, count_check_bb, loop_exit_bb);

  // Second check: is inner done?
  LLVMPositionBuilderAtEnd(builder, count_check_bb);
  // Load inner handle from slot
  LLVMValueRef inner_handle_for_done =
      LLVMBuildLoad2(builder, GENERIC_PTR, inner_handle_slot, "inner_handle");
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_for_done},
      1, "inner.is_done_before");
  LLVMBuildCondBr(builder, is_done_before, loop_exit_bb, loop_body_bb);

  // Resume inner
  LLVMPositionBuilderAtEnd(builder, loop_body_bb);
  LLVMValueRef inner_handle_for_resume =
      LLVMBuildLoad2(builder, GENERIC_PTR, inner_handle_slot, "inner_handle");
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_for_resume}, 1, "");

  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module),
      (LLVMValueRef[]){inner_handle_for_resume}, 1, "inner.is_done_after");
  LLVMBuildCondBr(builder, is_done_after, loop_exit_bb, get_value_bb);

  // Get value from inner and yield it
  LLVMPositionBuilderAtEnd(builder, get_value_bb);
  LLVMValueRef inner_handle_for_value =
      LLVMBuildLoad2(builder, GENERIC_PTR, inner_handle_slot, "inner_handle");

  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle_for_value,
                       LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  LLVMValueRef inner_promise_ptr = LLVMBuildBitCast(
      builder, inner_promise_raw, LLVMPointerType(llvm_yield_type, 0),
      "inner.promise.ptr");

  LLVMValueRef inner_value = LLVMBuildLoad2(builder, llvm_yield_type,
                                            inner_promise_ptr, "inner.value");

  // Store in our promise
  LLVMBuildStore(builder, inner_value, promise_alloca);

  // Decrement remaining counter
  LLVMValueRef remaining_now =
      LLVMBuildLoad2(builder, LLVMInt32Type(), remaining_slot, "remaining.now");

  LLVMValueRef remaining_dec =
      LLVMBuildSub(builder, remaining_now, LLVMConstInt(LLVMInt32Type(), 1, 0),
                   "remaining.dec");
  LLVMBuildStore(builder, remaining_dec, remaining_slot);

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
      LLVMAppendBasicBlock(wrapper_fn, "take.suspend_return");

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
  LLVMBuildBr(builder, loop_check_bb);

  // When count exhausted or inner done, finish
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

  // Alloca for frame size (not used here, but wrapper expects it)
  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  // Call the wrapper to create the take coroutine handle
  LLVMValueRef take_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){frame_size_alloca, count_val, inner_handle}, 3,
      "take.handle");

  // Create reset closure for coro_take
  // Closure args: {take_count (i32), inner_handle (ptr)}
  LLVMTypeRef closure_args_ty =
      LLVMStructType((LLVMTypeRef[]){LLVMInt32Type(), GENERIC_PTR}, 2, 0);

  LLVMValueRef reset_closure_fn =
      coro_take_reset_fn(promise_type, inner_prom_type, wrapper_fn_type,
                         wrapper_fn, module, builder);

  // Allocate and populate closure args struct
  LLVMValueRef args_struct = LLVMGetUndef(closure_args_ty);
  args_struct = LLVMBuildInsertValue(builder, args_struct, count_val, 0, "");
  args_struct = LLVMBuildInsertValue(builder, args_struct, inner_handle, 1, "");

  LLVMValueRef args_ptr_alloc = LLVMBuildMalloc(builder, closure_args_ty, "");
  LLVMBuildStore(builder, args_struct, args_ptr_alloc);

  // Write reset_fn and args_ptr into take_handle's promise
  LLVMValueRef take_prom_ptr = GET_PROMISE_PTR(take_handle, promise_type);
  PROMISE_SET_RESET_FN(take_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(take_prom_ptr, promise_type, args_ptr_alloc);

  return take_handle;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  // Handle is now just a pointer, not FAT_HANDLE
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
  Type *cor_type = ast->type;
  Type *yield_type = cor_type->data.T_CONS.args[0];
  LLVMTypeRef llvm_yield_type = type_to_llvm_type(yield_type, ctx, module);
  LLVMTypeRef prom_type = CORO_PROMISE_TYPE(llvm_yield_type);

  LLVMValueRef prom_ptr = GET_PROMISE_PTR(handle, prom_type);
  LLVMValueRef is_done_flag_ptr =
      LLVMBuildStructGEP2(builder, prom_type, prom_ptr, 1, "get_is_done_flag");
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
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_of_cor_list_%d",
           counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  // Outer loop blocks - iterate through list
  LLVMBasicBlockRef outer_loop_bb =
      LLVMAppendBasicBlock(wrapper_fn, "outer.loop");
  LLVMBasicBlockRef outer_loop_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "outer.loop.body");
  LLVMBasicBlockRef outer_loop_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "outer.loop.exit");

  // Inner loop blocks - yield from current coroutine
  LLVMBasicBlockRef inner_check_bb =
      LLVMAppendBasicBlock(wrapper_fn, "inner.check");
  LLVMBasicBlockRef inner_body_bb =
      LLVMAppendBasicBlock(wrapper_fn, "inner.body");
  LLVMBasicBlockRef inner_get_value_bb =
      LLVMAppendBasicBlock(wrapper_fn, "inner.get_value");
  LLVMBasicBlockRef inner_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "inner.resume");
  LLVMBasicBlockRef inner_exit_bb =
      LLVMAppendBasicBlock(wrapper_fn, "inner.exit");

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  // Promise holds the yielded element type T
  LLVMTypeRef promise_type = llvm_elem_type;
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Get the actual node type {Coroutine<T>, void*}
  LLVMTypeRef node_type = LLVMStructType(
      (LLVMTypeRef[]){llvm_coro_type, LLVMPointerType(LLVMVoidType(), 0)}, 2,
      0);

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

  LLVMValueRef inner_handle_loaded = LLVMBuildLoad2(
      builder, llvm_coro_type, inner_handle_alloca, "inner.handle.loaded");

  // Check if inner is done BEFORE resuming
  LLVMValueRef is_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){inner_handle_loaded}, 1,
      "inner.is_done_before");

  LLVMBuildCondBr(builder, is_done_before, inner_exit_bb, inner_body_bb);

  // === INNER LOOP BODY: Resume inner coroutine ===
  LLVMPositionBuilderAtEnd(builder, inner_body_bb);

  LLVMValueRef inner_handle_for_resume = LLVMBuildLoad2(
      builder, llvm_coro_type, inner_handle_alloca, "inner.handle.for_resume");

  // Resume the inner coroutine
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){inner_handle_for_resume}, 1, "");

  // Check if done AFTER resume
  LLVMValueRef is_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module),
      (LLVMValueRef[]){inner_handle_for_resume}, 1, "inner.is_done_after");

  // If done after resume, exit the inner loop (move to next list element)
  LLVMBuildCondBr(builder, is_done_after, inner_exit_bb, inner_get_value_bb);

  // === INNER GET VALUE: Read inner's promise and yield it ===
  LLVMPositionBuilderAtEnd(builder, inner_get_value_bb);

  LLVMValueRef inner_handle_for_promise = LLVMBuildLoad2(
      builder, llvm_coro_type, inner_handle_alloca, "inner.handle.for_promise");

  // Get inner coroutine's promise
  LLVMValueRef inner_promise_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){inner_handle_for_promise,
                       LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "inner.promise.raw");

  // Cast to correct type
  LLVMValueRef inner_promise_ptr =
      LLVMBuildBitCast(builder, inner_promise_raw,
                       LLVMPointerType(llvm_elem_type, 0), "inner.promise.ptr");

  // Load the value from inner's promise
  LLVMValueRef inner_value =
      LLVMBuildLoad2(builder, llvm_elem_type, inner_promise_ptr, "inner.value");

  // Store it in OUR promise (we're yielding this value)
  LLVMBuildStore(builder, inner_value, promise_alloca);

  // Suspend (yield this value to our caller)
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

  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), inner_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  // === INNER RESUME: When we're resumed, loop back to check for more values
  // ===
  LLVMPositionBuilderAtEnd(builder, inner_resume_bb);
  LLVMBuildBr(builder, inner_check_bb); // Loop back to inner check!

  // === INNER EXIT: Current coroutine exhausted, move to next list element ===
  LLVMPositionBuilderAtEnd(builder, inner_exit_bb);

  // Load the current list node
  LLVMValueRef current_for_next = LLVMBuildLoad2(
      builder, llvm_list_type, current_alloca, "current.for_next");

  // GEP to next field (field 1) and load
  LLVMValueRef next_ptr_ptr = LLVMBuildStructGEP2(
      builder, node_type, current_for_next, 1, "next.ptr.ptr");
  LLVMValueRef next_ptr = LLVMBuildLoad2(
      builder, LLVMPointerType(LLVMVoidType(), 0), next_ptr_ptr, "next.ptr");

  // Cast void* to proper list pointer type (ptr to node)
  LLVMValueRef next_typed =
      LLVMBuildBitCast(builder, next_ptr, llvm_list_type, "next.typed");
  LLVMBuildStore(builder, next_typed, current_alloca);

  LLVMBuildBr(builder,
              outer_loop_bb); // Back to outer loop to get next coroutine

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
LLVMValueRef cor_of_list_reset_fn(LLVMTypeRef promise_type,
                                  LLVMTypeRef wrapper_fn_type,
                                  LLVMValueRef wrapper_fn, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  // Reset closure signature: (ptr frame_size_out, ptr args_ptr) -> ptr handle
  // For cor_of_list, args_ptr is simply the original list pointer
  LLVMTypeRef reset_closure_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), GENERIC_PTR}, 2, 0);

  char reset_name[64];
  snprintf(reset_name, sizeof(reset_name), "coro_of_list.reset");

  LLVMValueRef reset_closure_fn =
      LLVMAddFunction(module, reset_name, reset_closure_type);
  LLVMSetLinkage(reset_closure_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef reset_prev = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef reset_entry =
      LLVMAppendBasicBlock(reset_closure_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, reset_entry);

  // Reset closure params: (ptr frame_size_out, ptr args_ptr)
  LLVMValueRef reset_frame_size_out = LLVMGetParam(reset_closure_fn, 0);
  LLVMValueRef reset_list_ptr = LLVMGetParam(reset_closure_fn, 1);

  // Call wrapper_fn(frame_size_out, list_ptr) to create new coroutine
  LLVMValueRef new_handle =
      LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn,
                     (LLVMValueRef[]){reset_frame_size_out, reset_list_ptr}, 2,
                     "new_list_handle");

  // Write reset_fn and args_ptr into the new handle's promise
  LLVMValueRef new_prom_ptr = GET_PROMISE_PTR(new_handle, promise_type);
  PROMISE_SET_RESET_FN(new_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(new_prom_ptr, promise_type, reset_list_ptr);

  LLVMBuildRet(builder, new_handle);

  LLVMPositionBuilderAtEnd(builder, reset_prev);
  return reset_closure_fn;
}

LLVMValueRef
cor_of_array_reset_fn(LLVMTypeRef promise_type, LLVMTypeRef wrapper_fn_type,
                      LLVMTypeRef llvm_array_type, LLVMValueRef wrapper_fn,
                      LLVMModuleRef module, LLVMBuilderRef builder) {
  // Reset closure signature: (ptr frame_size_out, ptr args_ptr) -> ptr handle
  // For cor_of_array, args_ptr points to a malloced array struct {size, data}
  LLVMTypeRef reset_closure_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), GENERIC_PTR}, 2, 0);

  char reset_name[64];
  snprintf(reset_name, sizeof(reset_name), "coro_of_array.reset");

  LLVMValueRef reset_closure_fn =
      LLVMAddFunction(module, reset_name, reset_closure_type);
  LLVMSetLinkage(reset_closure_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef reset_prev = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef reset_entry =
      LLVMAppendBasicBlock(reset_closure_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, reset_entry);

  // Reset closure params: (ptr frame_size_out, ptr args_ptr)
  LLVMValueRef reset_frame_size_out = LLVMGetParam(reset_closure_fn, 0);
  LLVMValueRef reset_args_raw = LLVMGetParam(reset_closure_fn, 1);

  // Cast args_ptr to pointer to array struct and load the array value
  LLVMValueRef array_ptr =
      LLVMBuildBitCast(builder, reset_args_raw,
                       LLVMPointerType(llvm_array_type, 0), "array_ptr");
  LLVMValueRef array_val =
      LLVMBuildLoad2(builder, llvm_array_type, array_ptr, "array_val");

  // Call wrapper_fn(frame_size_out, array_val) to create new coroutine
  LLVMValueRef new_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){reset_frame_size_out, array_val}, 2, "new_array_handle");

  // Write reset_fn and args_ptr into the new handle's promise
  LLVMValueRef new_prom_ptr = GET_PROMISE_PTR(new_handle, promise_type);
  PROMISE_SET_RESET_FN(new_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(new_prom_ptr, promise_type, reset_args_raw);

  LLVMBuildRet(builder, new_handle);

  LLVMPositionBuilderAtEnd(builder, reset_prev);
  return reset_closure_fn;
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

  // Create wrapper coroutine function that TAKES frame_size_out and list as
  // parameters
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), llvm_list_type}, 2,
      0);

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

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_elem_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null (not resettable)
  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

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

  // Write frame size to caller's out-param (param 0)
  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  // Get the list parameter (param 0 is frame_size_out, param 1 is the list)
  LLVMValueRef list_param = LLVMGetParam(wrapper_fn, 1);
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

  // Allocate space for frame size output
  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  // Call the wrapper function, passing frame_size_out and the list as arguments
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){frame_size_alloca, list_ptr}, 2, "list.coro.handle");

  // Create reset closure for list coroutine
  LLVMValueRef reset_closure_fn = cor_of_list_reset_fn(
      promise_type, wrapper_fn_type, wrapper_fn, module, builder);

  // Write reset_fn and args_ptr into coro_handle's promise
  // For cor_of_list, args_ptr is simply the original list pointer
  LLVMValueRef coro_prom_ptr = GET_PROMISE_PTR(coro_handle, promise_type);
  PROMISE_SET_RESET_FN(coro_prom_ptr, promise_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(coro_prom_ptr, promise_type, list_ptr);

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

  // Create wrapper coroutine function that TAKES frame_size_out and array as
  // parameters
  LLVMTypeRef wrapper_fn_type = LLVMFunctionType(
      GENERIC_PTR,
      (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0), llvm_array_type}, 2,
      0);

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

  LLVMTypeRef promise_struct_type = CORO_PROMISE_TYPE(llvm_elem_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_struct_type, "promise");

  // Initialize is_done flag to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_struct_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  // Initialize reset_fn and args_ptr to null (not resettable)
  PROMISE_SET_RESET_FN(promise_alloca, promise_struct_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_struct_type,
                       LLVMConstNull(GENERIC_PTR));

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

  // Write frame size to caller's out-param (param 0)
  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  // Get the array parameter (param 0 is frame_size_out, param 1 is the array)
  LLVMValueRef array_param = LLVMGetParam(wrapper_fn, 1);
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

  // Allocate space for frame size output
  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  // Call the wrapper function, passing frame_size_out and the array as
  // arguments
  LLVMValueRef coro_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){frame_size_alloca, array_val}, 2, "array.coro.handle");

  // Create reset closure for array coroutine
  LLVMValueRef reset_closure_fn =
      cor_of_array_reset_fn(promise_struct_type, wrapper_fn_type,
                            llvm_array_type, wrapper_fn, module, builder);

  // Malloc storage for the array struct and store the value there
  // (array is a struct value {size, data}, not a pointer)
  LLVMValueRef args_ptr_alloc =
      LLVMBuildMalloc(builder, llvm_array_type, "array_args");
  LLVMBuildStore(builder, array_val, args_ptr_alloc);

  // Write reset_fn and args_ptr into coro_handle's promise
  LLVMValueRef coro_prom_ptr =
      GET_PROMISE_PTR(coro_handle, promise_struct_type);
  PROMISE_SET_RESET_FN(coro_prom_ptr, promise_struct_type, reset_closure_fn);
  PROMISE_SET_ARGS_PTR(coro_prom_ptr, promise_struct_type, args_ptr_alloc);

  return coro_handle;
}

LLVMValueRef PlayRoutineQuantHandler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  print_ast(ast);
  return LLVMConstReal(LLVMDoubleType(), 0.);
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

  // Determine yield type from the coroutine: if tuple, field 0 is the duration
  Type *cor_yield_type_t = cor_ast->type->data.T_CONS.args[0];
  bool yield_is_tuple =
      (cor_yield_type_t->kind == T_CONS &&
       strcmp(cor_yield_type_t->data.T_CONS.name, TYPE_NAME_TUPLE) == 0);
  LLVMTypeRef yield_type = type_to_llvm_type(cor_yield_type_t, ctx, module);

  LLVMValueRef resume_result =
      codegen_handle_resume(_handle, yield_type, ctx, module, builder);

  LLVMValueRef result_tag =
      LLVMBuildExtractValue(builder, resume_result, 0, "tag");

  LLVMValueRef is_done =
      LLVMBuildICmp(builder, LLVMIntEQ, result_tag,
                    LLVMConstInt(LLVMInt8Type(), 1, 0), "tag_eq_1");

  LLVMBuildCondBr(builder, is_done, finished, not_finished);

  // coroutine not finished - extract dur from yielded value and schedule next
  LLVMPositionBuilderAtEnd(builder, not_finished);
  LLVMValueRef promise_ptr_raw = GET_PROMISE_PTR_RAW(_handle);
  LLVMValueRef yield_ptr = LLVMBuildBitCast(
      builder, promise_ptr_raw, LLVMPointerType(yield_type, 0), "promise.ptr");

  LLVMValueRef yielded_raw =
      LLVMBuildLoad2(builder, yield_type, yield_ptr, "yielded.raw");

  // If the yield type is a tuple, field 0 is the duration double
  LLVMValueRef dur_value;
  if (yield_is_tuple) {
    dur_value = LLVMBuildExtractValue(builder, yielded_raw, 0, "dur");
  } else {
    dur_value = yielded_raw;
  }

  LLVMValueRef scheduler_call =
      LLVMBuildCall2(builder, schedule_event_type, schedule_event,
                     (LLVMValueRef[]){
                         _u64ts,
                         dur_value,
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

LLVMValueRef CurrentCorHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  fprintf(stderr, "TODO: CurrentCorHandler not yet implemented\n");
  return NULL;
}

LLVMValueRef CorUnwrapOrEndHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  CoroutineCtx *coro_ctx = (CoroutineCtx *)ctx->coro_ctx;

  if (!coro_ctx) {
    fprintf(stderr,
            "Error: cor_unwrap_or_end used outside of coroutine context\n");
    return NULL;
  }

  LLVMValueRef opt_val =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
  LLVMValueRef current_fn = LLVMGetBasicBlockParent(current_bb);

  LLVMBasicBlockRef is_none_bb =
      LLVMAppendBasicBlock(current_fn, "opt_is_none");
  LLVMBasicBlockRef is_some_bb =
      LLVMAppendBasicBlock(current_fn, "opt_is_some");

  LLVMValueRef is_none = codegen_option_is_none(opt_val, builder);
  LLVMBuildCondBr(builder, is_none, is_none_bb, is_some_bb);

  LLVMPositionBuilderAtEnd(builder, is_none_bb);
  coro_emit_final_suspend(ctx, module, builder, coro_ctx->coro_handle,
                          current_fn, coro_ctx->cleanup_bb,
                          coro_ctx->suspend_bb);

  LLVMPositionBuilderAtEnd(builder, is_some_bb);
  LLVMValueRef inner_value =
      LLVMBuildExtractValue(builder, opt_val, 1, "unwrapped_value");

  return inner_value;
}

// Ast *optimise_coro_combinators(Ast *ast) {
//   printf("detect coroutine manipulation optimisation opportunities\n");
//
//   // Ast *apps[]
//
//   int n = 0;
//   Ast *_ast = ast;
//   while (_ast->tag == AST_APPLICATION) {
//     _ast =
//         _ast->data.AST_APPLICATION.args + (_ast->data.AST_APPLICATION.len -
//         1);
//     n++;
//   }
//   Ast *apps[n];
//
//   int i = 0;
//   while (ast->tag == AST_APPLICATION) {
//     apps[i] = ast;
//     ast = ast->data.AST_APPLICATION.args + (ast->data.AST_APPLICATION.len -
//     1); i++;
//   }
//   for (int i = 0; i < n; i++) {
//     print_ast(apps[i]);
//   }
//
//   return NULL;
// }

// Helper to check function name
static bool is_fn_call(Ast *ast, const char *name) {
  if (ast->tag != AST_APPLICATION)
    return false;
  Ast *fn = ast->data.AST_APPLICATION.function;
  if (fn->tag != AST_IDENTIFIER)
    return false;
  return strcmp(fn->data.AST_IDENTIFIER.value, name) == 0;
}

// Check if this is a combinator we can push loop through
static bool is_transparent_coroutine_combinator(Ast *ast) {
  if (ast->tag != AST_APPLICATION)
    return false;
  Ast *fn = ast->data.AST_APPLICATION.function;
  if (fn->tag != AST_IDENTIFIER)
    return false;
  const char *name = fn->data.AST_IDENTIFIER.value;
  return strcmp(name, "cor_map") == 0;
  // / || strcmp(name, "filter") == 0;
}

// Fuse consecutive cor_map calls into a single cor_map with fn_composition
// cor_map(h, cor_map(g, cor_map(f, X))) → cor_map(fn_composition(f, g, h), X)
static Ast *fuse_cor_maps(Ast *ast) {
  if (!is_fn_call(ast, "cor_map") || ast->data.AST_APPLICATION.len != 2) {
    return NULL;
  }

  // Collect all consecutive cor_map functions
  Ast *funcs[64];
  int num_funcs = 0;
  Ast *current = ast;

  while (is_fn_call(current, "cor_map") &&
         current->data.AST_APPLICATION.len == 2) {
    funcs[num_funcs++] = &current->data.AST_APPLICATION.args[0];
    current = &current->data.AST_APPLICATION.args[1];
    if (num_funcs >= 64)
      break;
  }

  if (num_funcs < 2)
    return NULL; // Nothing to fuse

  Ast *source = current;

  // Build fn_composition(f, g, h) - innermost first, outermost last
  // funcs[0] is outermost (h), funcs[num_funcs-1] is innermost (f)
  // We want fn_composition(f, g, h) to apply f first, then g, then h
  Ast *comp_args = malloc(sizeof(Ast) * num_funcs);
  for (int i = 0; i < num_funcs; i++) {
    comp_args[i] = *funcs[num_funcs - 1 - i]; // reverse: innermost first
  }

  // print_ast(funcs[num_funcs - 1])
  Type *innermost_type = comp_args[0].type;
  Type *innermost_input_type = innermost_type->data.T_FN.from;

  Type *outermost_type = funcs[0]->type;
  Type *outermost_ret_type = fn_return_type(outermost_type);

  Type *fused_type = type_fn(innermost_input_type, outermost_ret_type);

  Ast *composition = Ast_new(AST_APPLICATION);
  composition->type = fused_type;
  composition->data.AST_APPLICATION.function =
      ast_identifier((ObjString){"fn_composition", 14});

  // TODO: this is a little wrong but type can just not be NULL
  composition->data.AST_APPLICATION.function->type = fused_type;

  composition->data.AST_APPLICATION.args = comp_args;
  composition->data.AST_APPLICATION.len = num_funcs;

  // Build cor_map(composition, source)
  Ast *fused_args = malloc(sizeof(Ast) * 2);
  fused_args[0] = *composition;
  fused_args[1] = *source;

  Ast *fused = Ast_new(AST_APPLICATION);
  fused->data.AST_APPLICATION.function = ast->data.AST_APPLICATION.function;
  fused->data.AST_APPLICATION.args = fused_args;
  fused->data.AST_APPLICATION.len = 2;
  fused->type = ast->type;

  return fused;
}

// Recursively optimize coroutine combinators anywhere in the AST
// Returns optimized AST, or NULL if no optimization was possible
// Swaps: cor_loop(transparent(args..., X)) → transparent(args..., cor_loop(X))
Ast *optimise_coro_combinators(Ast *ast) {
  if (ast == NULL || ast->tag != AST_APPLICATION) {
    return NULL;
  }

  // Check if THIS node is cor_loop(transparent_combinator(args..., X))
  if (is_fn_call(ast, "cor_loop") && ast->data.AST_APPLICATION.len == 1) {
    Ast *inner = &ast->data.AST_APPLICATION.args[0];
    if (is_transparent_coroutine_combinator(inner)) {
      Ast *loop_fn = ast->data.AST_APPLICATION.function;
      size_t inner_len = inner->data.AST_APPLICATION.len;
      Ast *innermost = &inner->data.AST_APPLICATION.args[inner_len - 1];

      // Build cor_loop(X)
      Ast *loop_arg = malloc(sizeof(Ast));
      *loop_arg = *innermost;
      Ast *new_loop = Ast_new(AST_APPLICATION);
      new_loop->data.AST_APPLICATION.function = loop_fn;
      new_loop->data.AST_APPLICATION.args = loop_arg;
      new_loop->data.AST_APPLICATION.len = 1;
      new_loop->type = ast->type;

      // Build transparent(args..., cor_loop(X))
      Ast *new_args = malloc(sizeof(Ast) * inner_len);
      for (size_t i = 0; i < inner_len - 1; i++) {
        new_args[i] = inner->data.AST_APPLICATION.args[i];
      }
      new_args[inner_len - 1] = *new_loop;

      Ast *new_outer = Ast_new(AST_APPLICATION);
      new_outer->data.AST_APPLICATION.function =
          inner->data.AST_APPLICATION.function;
      new_outer->data.AST_APPLICATION.args = new_args;
      new_outer->data.AST_APPLICATION.len = inner_len;
      new_outer->type = inner->type;

      // Recursively optimize (will swap again if needed)
      Ast *further = optimise_coro_combinators(new_outer);
      return further ? further : new_outer;
    }
  }

  // Recursively try to optimize arguments
  size_t len = ast->data.AST_APPLICATION.len;
  Ast *new_args = NULL;
  bool changed = false;

  for (size_t i = 0; i < len; i++) {
    Ast *arg = &ast->data.AST_APPLICATION.args[i];
    Ast *opt = optimise_coro_combinators(arg);

    if (opt != NULL) {
      if (!changed) {
        new_args = malloc(sizeof(Ast) * len);
        for (size_t j = 0; j < i; j++) {
          new_args[j] = ast->data.AST_APPLICATION.args[j];
        }
        changed = true;
      }
      new_args[i] = *opt;
    } else if (changed) {
      new_args[i] = ast->data.AST_APPLICATION.args[i];
    }
  }

  Ast *result = ast;
  if (changed) {
    Ast *new_app = Ast_new(AST_APPLICATION);
    new_app->type = ast->type;
    new_app->data.AST_APPLICATION.function = ast->data.AST_APPLICATION.function;
    new_app->data.AST_APPLICATION.args = new_args;
    new_app->data.AST_APPLICATION.len = len;
    result = new_app;
  }

  // Try to fuse consecutive cor_map calls
  Ast *fused = fuse_cor_maps(result);
  if (fused) {
    return fused;
  }

  return changed ? result : NULL;
}

LLVMValueRef CorZipHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  // Get the two coroutine arguments
  Ast *a_ast = ast->data.AST_APPLICATION.args;
  Ast *b_ast = ast->data.AST_APPLICATION.args + 1;

  // Extract yield types from Coroutine<A> and Coroutine<B>
  Type *a_yield_type = a_ast->type->data.T_CONS.args[0];
  Type *b_yield_type = b_ast->type->data.T_CONS.args[0];

  // Compute result yield type using concat_tuples spreading semantics
  Type *result_yield_type = concat_tuples(a_yield_type, b_yield_type);

  // Determine if either yield type is a tuple (for field spreading)
  bool a_is_tuple =
      (a_yield_type->kind == T_CONS &&
       strcmp(a_yield_type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0);
  bool b_is_tuple =
      (b_yield_type->kind == T_CONS &&
       strcmp(b_yield_type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0);
  int a_lena = a_is_tuple ? a_yield_type->data.T_CONS.num_args : 1;
  int b_lenb = b_is_tuple ? b_yield_type->data.T_CONS.num_args : 1;

  // Convert to LLVM types
  LLVMTypeRef llvm_a_type = type_to_llvm_type(a_yield_type, ctx, module);
  LLVMTypeRef llvm_b_type = type_to_llvm_type(b_yield_type, ctx, module);
  LLVMTypeRef llvm_result_type =
      type_to_llvm_type(result_yield_type, ctx, module);

  // Codegen both coroutine handles in the current block
  LLVMValueRef a_handle = codegen(a_ast, ctx, module, builder);
  LLVMValueRef b_handle = codegen(b_ast, ctx, module, builder);

  // Create wrapper coroutine: (i64* frame_size_out, i8* a, i8* b) -> i8*
  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){LLVMPointerType(LLVMInt64Type(), 0),
                                       GENERIC_PTR, GENERIC_PTR},
                       3, 0);
  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, "coro_zip", wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY BLOCK ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_result_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  // Initialize is_done to false
  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");

  LLVMValueRef frame_size_out_param = LLVMGetParam(wrapper_fn, 0);
  LLVMBuildStore(builder, size, frame_size_out_param);

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

  LLVMValueRef a_handle_param = LLVMGetParam(wrapper_fn, 1);
  LLVMSetValueName(a_handle_param, "a_handle");
  LLVMValueRef b_handle_param = LLVMGetParam(wrapper_fn, 2);
  LLVMSetValueName(b_handle_param, "b_handle");

  // === ZIP LOOP ===
  LLVMBasicBlockRef zip_check_a_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.check_a");
  LLVMBasicBlockRef zip_check_b_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.check_b");
  LLVMBasicBlockRef zip_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.resume");
  LLVMBasicBlockRef zip_check_a_done_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.check_a_done");
  LLVMBasicBlockRef zip_check_b_done_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.check_b_done");
  LLVMBasicBlockRef zip_values_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.values");
  LLVMBasicBlockRef zip_loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zip.loop_resume");
  LLVMBasicBlockRef zip_exit_bb = LLVMAppendBasicBlock(wrapper_fn, "zip.exit");

  LLVMBuildBr(builder, zip_check_a_bb);

  // zip.check_a: if a done before resume → exit
  LLVMPositionBuilderAtEnd(builder, zip_check_a_bb);
  LLVMValueRef a_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){a_handle_param}, 1,
      "a.done.before");
  LLVMBuildCondBr(builder, a_done_before, zip_exit_bb, zip_check_b_bb);

  // zip.check_b: if b done before resume → exit
  LLVMPositionBuilderAtEnd(builder, zip_check_b_bb);
  LLVMValueRef b_done_before = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){b_handle_param}, 1,
      "b.done.before");
  LLVMBuildCondBr(builder, b_done_before, zip_exit_bb, zip_resume_bb);

  // zip.resume: resume both coroutines
  LLVMPositionBuilderAtEnd(builder, zip_resume_bb);
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){a_handle_param}, 1, "");
  LLVMBuildCall2(builder,
                 LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
                 get_coro_resume_intrinsic(module),
                 (LLVMValueRef[]){b_handle_param}, 1, "");
  LLVMBuildBr(builder, zip_check_a_done_bb);

  // zip.check_a_done: if a done after resume → exit
  LLVMPositionBuilderAtEnd(builder, zip_check_a_done_bb);
  LLVMValueRef a_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){a_handle_param}, 1,
      "a.done.after");
  LLVMBuildCondBr(builder, a_done_after, zip_exit_bb, zip_check_b_done_bb);

  // zip.check_b_done: if b done after resume → exit
  LLVMPositionBuilderAtEnd(builder, zip_check_b_done_bb);
  LLVMValueRef b_done_after = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
      get_coro_done_intrinsic(module), (LLVMValueRef[]){b_handle_param}, 1,
      "b.done.after");
  LLVMBuildCondBr(builder, b_done_after, zip_exit_bb, zip_values_bb);

  // zip.values: read both promises, build combined value, yield
  LLVMPositionBuilderAtEnd(builder, zip_values_bb);

  // Read a's yield value from its promise (field 0)
  LLVMValueRef a_prom_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){a_handle_param, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "a.prom.raw");
  LLVMValueRef a_prom_ptr = LLVMBuildBitCast(
      builder, a_prom_raw, LLVMPointerType(llvm_a_type, 0), "a.prom.ptr");
  LLVMValueRef a_val =
      LLVMBuildLoad2(builder, llvm_a_type, a_prom_ptr, "a.val");

  // Read b's yield value from its promise (field 0)
  LLVMValueRef b_prom_raw = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
      get_coro_promise_intrinsic(module),
      (LLVMValueRef[]){b_handle_param, LLVMConstInt(LLVMInt32Type(), 0, 0),
                       LLVMConstInt(LLVMInt1Type(), 0, 0)},
      3, "b.prom.raw");
  LLVMValueRef b_prom_ptr = LLVMBuildBitCast(
      builder, b_prom_raw, LLVMPointerType(llvm_b_type, 0), "b.prom.ptr");
  LLVMValueRef b_val =
      LLVMBuildLoad2(builder, llvm_b_type, b_prom_ptr, "b.val");

  // Build combined result using concat_tuples spreading: spread tuples, insert
  // scalars
  LLVMValueRef result_val = LLVMGetUndef(llvm_result_type);
  if (a_is_tuple) {
    for (int i = 0; i < a_lena; i++) {
      LLVMValueRef field = LLVMBuildExtractValue(builder, a_val, i, "a.field");
      result_val = LLVMBuildInsertValue(builder, result_val, field, i, "res.a");
    }
  } else {
    result_val = LLVMBuildInsertValue(builder, result_val, a_val, 0, "res.a");
  }
  if (b_is_tuple) {
    for (int i = 0; i < b_lenb; i++) {
      LLVMValueRef field = LLVMBuildExtractValue(builder, b_val, i, "b.field");
      result_val =
          LLVMBuildInsertValue(builder, result_val, field, a_lena + i, "res.b");
    }
  } else {
    result_val =
        LLVMBuildInsertValue(builder, result_val, b_val, a_lena, "res.b");
  }

  // Store combined value to promise (overwrites field 0 at offset 0)
  LLVMBuildStore(builder, result_val, promise_alloca);

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
      LLVMAppendBasicBlock(wrapper_fn, "zip.suspend_return");
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0),
              zip_loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, zip_loop_resume_bb);
  LLVMBuildBr(builder, zip_check_a_bb);

  // zip.exit: final suspend when either coroutine is exhausted
  LLVMPositionBuilderAtEnd(builder, zip_exit_bb);

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

  // === BACK IN ORIGINAL BLOCK ===
  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");

  LLVMValueRef zip_handle = LLVMBuildCall2(
      builder, wrapper_fn_type, wrapper_fn,
      (LLVMValueRef[]){frame_size_alloca, a_handle, b_handle}, 3, "zip.handle");

  return zip_handle;
}

static bool is_void_arg_function_type(Type *t) {
  return t && t->kind == T_FN && t->data.T_FN.from &&
         t->data.T_FN.from->kind == T_VOID;
}

static LLVMValueRef call_zero_arg_function_value(Type *fn_type,
                                                 LLVMValueRef fn_value,
                                                 JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder) {
  // Closure runtime path: value is {fn, args_ptr}.
  if (is_closure(fn_type) &&
      LLVMGetTypeKind(LLVMTypeOf(fn_value)) == LLVMStructTypeKind &&
      LLVMCountStructElementTypes(LLVMTypeOf(fn_value)) == 2) {

    LLVMValueRef closure_fn =
        LLVMBuildExtractValue(builder, fn_value, 0, "zips.closure.fn");
    LLVMValueRef closure_args_ptr =
        LLVMBuildExtractValue(builder, fn_value, 1, "zips.closure.args_ptr");

    LLVMTypeRef env_type = closure_record_type(fn_type, ctx, module);
    LLVMTypeRef clos_impl_type =
        closure_fn_type(fn_type, env_type, ctx, module);
    LLVMTypeRef clos_impl_ptr_type = LLVMPointerType(clos_impl_type, 0);
    LLVMTypeRef env_ptr_type = LLVMPointerType(env_type, 0);

    if (LLVMTypeOf(closure_fn) != clos_impl_ptr_type) {
      closure_fn = LLVMBuildBitCast(builder, closure_fn, clos_impl_ptr_type,
                                    "zips.closure.fn.cast");
    }

    if (LLVMTypeOf(closure_args_ptr) != env_ptr_type) {
      closure_args_ptr = LLVMBuildBitCast(
          builder, closure_args_ptr, env_ptr_type, "zips.closure.args.cast");
    }

    return LLVMBuildCall2(builder, clos_impl_type, closure_fn,
                          (LLVMValueRef[]){closure_args_ptr}, 1,
                          "zips.closure.call0");
  }

  // Plain function pointer path: () -> T
  LLVMTypeRef llvm_fn_type = type_to_llvm_type(fn_type, ctx, module);
  LLVMTypeRef llvm_fn_ptr_type = LLVMPointerType(llvm_fn_type, 0);
  LLVMValueRef callee = fn_value;
  if (LLVMTypeOf(callee) != llvm_fn_ptr_type) {
    callee =
        LLVMBuildBitCast(builder, callee, llvm_fn_ptr_type, "zips.fn.cast");
  }
  return LLVMBuildCall2(builder, llvm_fn_type, callee, NULL, 0,
                        "zips.fn.call0");
}

LLVMValueRef CorZipStructHandler(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *struct_ast = ast->data.AST_APPLICATION.args;
  Type *struct_type = struct_ast ? struct_ast->type : NULL;

  if (!struct_type || struct_type->kind != T_CONS) {
    fprintf(stderr, "Error: cor_zip_struct expects tuple/record argument\n");
    print_location(ast);
    return NULL;
  }

  if (!ast->type || !is_coroutine_type(ast->type)) {
    fprintf(stderr, "Error: cor_zip_struct result type must be coroutine\n");
    print_location(ast);
    return NULL;
  }

  int num_fields = struct_type->data.T_CONS.num_args;
  Type *result_yield_type = ast->type->data.T_CONS.args[0];
  LLVMTypeRef llvm_result_type =
      type_to_llvm_type(result_yield_type, ctx, module);

  // Evaluate input struct and extract each field value once in caller context.
  LLVMValueRef input_struct_val = codegen(struct_ast, ctx, module, builder);
  LLVMTypeRef llvm_input_struct_type =
      type_to_llvm_type(struct_type, ctx, module);
  LLVMValueRef input_fields[num_fields];
  for (int i = 0; i < num_fields; i++) {
    input_fields[i] = codegen_tuple_access(i, input_struct_val,
                                           llvm_input_struct_type, builder);
  }

  // Build wrapper signature from actual runtime field value types.
  LLVMTypeRef wrapper_param_types[1 + num_fields];
  wrapper_param_types[0] = LLVMPointerType(LLVMInt64Type(), 0);
  for (int i = 0; i < num_fields; i++) {
    wrapper_param_types[i + 1] = LLVMTypeOf(input_fields[i]);
  }

  // Track which fields are coroutines.
  int coro_field_indices[num_fields > 0 ? num_fields : 1];
  int num_coroutines = 0;
  for (int i = 0; i < num_fields; i++) {
    if (is_coroutine_type(struct_type->data.T_CONS.args[i])) {
      coro_field_indices[num_coroutines++] = i;
    }
  }

  LLVMTypeRef wrapper_fn_type =
      LLVMFunctionType(GENERIC_PTR, wrapper_param_types, 1 + num_fields, 0);

  static unsigned long zip_struct_counter = 0;
  char wrapper_name[64];
  snprintf(wrapper_name, sizeof(wrapper_name), "coro_zip_struct_%lu",
           zip_struct_counter++);

  LLVMValueRef wrapper_fn =
      LLVMAddFunction(module, wrapper_name, wrapper_fn_type);
  LLVMSetLinkage(wrapper_fn, LLVMExternalLinkage);

  COROUTINE_ATTR_MARKING(wrapper_fn)
  COROUTINE_BASIC_BLOCKS(wrapper_fn)

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  // === ENTRY ===
  LLVMPositionBuilderAtEnd(builder, entry_bb);

  LLVMTypeRef promise_type = CORO_PROMISE_TYPE(llvm_result_type);
  LLVMValueRef promise_alloca =
      LLVMBuildAlloca(builder, promise_type, "promise");

  LLVMValueRef is_done_gep = LLVMBuildStructGEP2(
      builder, promise_type, promise_alloca, 1, "is_done_ptr");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt1Type(), 0, 0), is_done_gep);

  PROMISE_SET_RESET_FN(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));
  PROMISE_SET_ARGS_PTR(promise_alloca, promise_type,
                       LLVMConstNull(GENERIC_PTR));

  LLVMValueRef id = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_id_intrinsic(module)),
      get_coro_id_intrinsic(module),
      (LLVMValueRef[]){LLVMConstInt(LLVMInt32Type(), 0, 0), promise_alloca,
                       LLVMConstNull(GENERIC_PTR), LLVMConstNull(GENERIC_PTR)},
      4, "coro.id");

  LLVMValueRef size = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_size_intrinsic(module)),
      get_coro_size_intrinsic(module), NULL, 0, "coro.size");
  LLVMBuildStore(builder, size, LLVMGetParam(wrapper_fn, 0));

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

  // === START ===
  LLVMPositionBuilderAtEnd(builder, start_bb);

  // Persist every field in coroutine frame state.
  LLVMValueRef field_slots[num_fields > 0 ? num_fields : 1];
  for (int i = 0; i < num_fields; i++) {
    LLVMTypeRef slot_ty = wrapper_param_types[i + 1];
    field_slots[i] = LLVMBuildAlloca(builder, slot_ty, "zips.field.slot");
    LLVMBuildStore(builder, LLVMGetParam(wrapper_fn, i + 1), field_slots[i]);
  }

  LLVMBasicBlockRef check_before_bbs[num_fields > 0 ? num_fields : 1];
  LLVMBasicBlockRef check_after_bbs[num_fields > 0 ? num_fields : 1];
  LLVMBasicBlockRef resume_bb = LLVMAppendBasicBlock(wrapper_fn, "zips.resume");
  LLVMBasicBlockRef values_bb = LLVMAppendBasicBlock(wrapper_fn, "zips.values");
  LLVMBasicBlockRef loop_resume_bb =
      LLVMAppendBasicBlock(wrapper_fn, "zips.loop_resume");
  LLVMBasicBlockRef exit_bb = LLVMAppendBasicBlock(wrapper_fn, "zips.exit");

  for (int i = 0; i < num_coroutines; i++) {
    char pre_name[48];
    char post_name[48];
    snprintf(pre_name, sizeof(pre_name), "zips.check_before.%d", i);
    snprintf(post_name, sizeof(post_name), "zips.check_after.%d", i);
    check_before_bbs[i] = LLVMAppendBasicBlock(wrapper_fn, pre_name);
    check_after_bbs[i] = LLVMAppendBasicBlock(wrapper_fn, post_name);
  }

  LLVMBuildBr(builder, num_coroutines > 0 ? check_before_bbs[0] : values_bb);

  // Pre-resume done checks: any done => whole zip_struct done.
  for (int i = 0; i < num_coroutines; i++) {
    LLVMPositionBuilderAtEnd(builder, check_before_bbs[i]);
    int field_idx = coro_field_indices[i];

    LLVMValueRef h_val =
        LLVMBuildLoad2(builder, wrapper_param_types[field_idx + 1],
                       field_slots[field_idx], "zips.coro.h.pre");
    LLVMValueRef h = h_val;
    if (LLVMTypeOf(h) != GENERIC_PTR) {
      h = LLVMBuildBitCast(builder, h, GENERIC_PTR, "zips.coro.h.pre.cast");
    }

    LLVMValueRef done_before = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
        get_coro_done_intrinsic(module), (LLVMValueRef[]){h}, 1, "done.before");

    LLVMBuildCondBr(builder, done_before, exit_bb,
                    (i + 1 < num_coroutines) ? check_before_bbs[i + 1]
                                             : resume_bb);
  }

  // Resume all coroutine fields.
  LLVMPositionBuilderAtEnd(builder, resume_bb);
  for (int i = 0; i < num_coroutines; i++) {
    int field_idx = coro_field_indices[i];
    LLVMValueRef h_val =
        LLVMBuildLoad2(builder, wrapper_param_types[field_idx + 1],
                       field_slots[field_idx], "zips.coro.h");
    LLVMValueRef h = h_val;
    if (LLVMTypeOf(h) != GENERIC_PTR) {
      h = LLVMBuildBitCast(builder, h, GENERIC_PTR, "zips.coro.h.cast");
    }
    LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_resume_intrinsic(module)),
        get_coro_resume_intrinsic(module), (LLVMValueRef[]){h}, 1, "");
  }
  LLVMBuildBr(builder, num_coroutines > 0 ? check_after_bbs[0] : values_bb);

  // Post-resume done checks.
  for (int i = 0; i < num_coroutines; i++) {
    LLVMPositionBuilderAtEnd(builder, check_after_bbs[i]);
    int field_idx = coro_field_indices[i];

    LLVMValueRef h_val =
        LLVMBuildLoad2(builder, wrapper_param_types[field_idx + 1],
                       field_slots[field_idx], "zips.coro.h.post");
    LLVMValueRef h = h_val;
    if (LLVMTypeOf(h) != GENERIC_PTR) {
      h = LLVMBuildBitCast(builder, h, GENERIC_PTR, "zips.coro.h.post.cast");
    }

    LLVMValueRef done_after = LLVMBuildCall2(
        builder, LLVMGlobalGetValueType(get_coro_done_intrinsic(module)),
        get_coro_done_intrinsic(module), (LLVMValueRef[]){h}, 1, "done.after");

    LLVMBuildCondBr(builder, done_after, exit_bb,
                    (i + 1 < num_coroutines) ? check_after_bbs[i + 1]
                                             : values_bb);
  }

  // values: build transformed yield payload from scalar/function/coroutine
  // fields.
  LLVMPositionBuilderAtEnd(builder, values_bb);
  LLVMValueRef result_val = LLVMGetUndef(llvm_result_type);

  for (int i = 0; i < num_fields; i++) {
    Type *field_type = struct_type->data.T_CONS.args[i];
    LLVMValueRef field_val;

    if (is_coroutine_type(field_type)) {
      Type *yield_t = field_type->data.T_CONS.args[0];
      LLVMTypeRef llvm_yield_t = type_to_llvm_type(yield_t, ctx, module);

      LLVMValueRef h_val = LLVMBuildLoad2(builder, wrapper_param_types[i + 1],
                                          field_slots[i], "zips.coro.h.val");
      LLVMValueRef h = h_val;
      if (LLVMTypeOf(h) != GENERIC_PTR) {
        h = LLVMBuildBitCast(builder, h, GENERIC_PTR, "zips.coro.h.val.cast");
      }

      LLVMValueRef prom_raw = LLVMBuildCall2(
          builder, LLVMGlobalGetValueType(get_coro_promise_intrinsic(module)),
          get_coro_promise_intrinsic(module),
          (LLVMValueRef[]){h, LLVMConstInt(LLVMInt32Type(), 0, 0),
                           LLVMConstInt(LLVMInt1Type(), 0, 0)},
          3, "field.prom.raw");
      LLVMValueRef prom_ptr =
          LLVMBuildBitCast(builder, prom_raw, LLVMPointerType(llvm_yield_t, 0),
                           "field.prom.ptr");
      field_val =
          LLVMBuildLoad2(builder, llvm_yield_t, prom_ptr, "field.prom.val");

    } else if (is_void_arg_function_type(field_type)) {
      LLVMValueRef fn_val = LLVMBuildLoad2(builder, wrapper_param_types[i + 1],
                                           field_slots[i], "zips.fn.val");
      field_val = call_zero_arg_function_value(field_type, fn_val, ctx, module,
                                               builder);

    } else {
      LLVMTypeRef llvm_scalar = type_to_llvm_type(field_type, ctx, module);
      field_val = LLVMBuildLoad2(builder, llvm_scalar, field_slots[i],
                                 "zips.scalar.val");
    }

    result_val =
        LLVMBuildInsertValue(builder, result_val, field_val, i, "zips.ins");
  }

  LLVMBuildStore(builder, result_val, promise_alloca);

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
      LLVMAppendBasicBlock(wrapper_fn, "zips.suspend_return");
  LLVMValueRef switch_inst =
      LLVMBuildSwitch(builder, suspend_result, suspend_return_bb, 2);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 0, 0), loop_resume_bb);
  LLVMAddCase(switch_inst, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, loop_resume_bb);
  LLVMBuildBr(builder, num_coroutines > 0 ? check_before_bbs[0] : values_bb);

  // exit: final suspend when any inner coroutine is exhausted.
  LLVMPositionBuilderAtEnd(builder, exit_bb);
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
      LLVMAppendBasicBlock(wrapper_fn, "zips.final_return");
  LLVMValueRef final_switch =
      LLVMBuildSwitch(builder, final_suspend, suspend_bb, 2);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 0, 0),
              final_return_bb);
  LLVMAddCase(final_switch, LLVMConstInt(LLVMInt8Type(), 1, 0), cleanup_bb);

  LLVMPositionBuilderAtEnd(builder, final_return_bb);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, cleanup_bb);
  LLVMValueRef mem = LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_free_intrinsic(module)),
      get_coro_free_intrinsic(module), (LLVMValueRef[]){id, handle}, 2,
      "coro.free");
  LLVMBuildFree(builder, mem);
  LLVMBuildBr(builder, suspend_bb);

  LLVMPositionBuilderAtEnd(builder, suspend_bb);
  LLVMBuildCall2(
      builder, LLVMGlobalGetValueType(get_coro_end_intrinsic(module)),
      get_coro_end_intrinsic(module),
      (LLVMValueRef[]){handle, LLVMConstInt(LLVMInt1Type(), 0, 0)}, 2, "");
  LLVMBuildRet(builder, handle);

  // === back to caller ===
  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef frame_size_alloca =
      LLVMBuildAlloca(builder, LLVMInt64Type(), "frame_size.ignored");
  LLVMValueRef call_args[1 + num_fields];
  call_args[0] = frame_size_alloca;
  for (int i = 0; i < num_fields; i++) {
    call_args[i + 1] = input_fields[i];
  }

  return LLVMBuildCall2(builder, wrapper_fn_type, wrapper_fn, call_args,
                        1 + num_fields, "zip_struct.handle");
}
