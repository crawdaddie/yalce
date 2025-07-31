#include "./coroutine_extensions.h"
#include "types.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  Type *promise_type = fn_return_type(ast->md);
  LLVMTypeRef ptype = type_to_llvm_type(promise_type, ctx, module);
  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(ptype);
  LLVMValueRef coro =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMTypeRef loop_wrapper_type = PTR_ID_FUNC_TYPE(coro_obj_type);

  LLVMValueRef loop_wrapper_fn =
      LLVMAddFunction(module, "coro_loop_wrapper", loop_wrapper_type);
  LLVMSetLinkage(loop_wrapper_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef entry_block =
      LLVMAppendBasicBlock(loop_wrapper_fn, "entry");
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, entry_block);
  LLVMValueRef wrapper_coro = LLVMGetParam(loop_wrapper_fn, 0);

  LLVMValueRef inner_coro = LLVMBuildBitCast(
      builder, coro_state(wrapper_coro, coro_obj_type, builder),
      LLVMPointerType(coro_obj_type, 0), "bitcast_generic_state_ptr");
  CoroutineCtx tmp_ctx = {.coro_obj_type = coro_obj_type};

  coro_advance(inner_coro, &tmp_ctx, builder);
  LLVMValueRef p = coro_promise(inner_coro, coro_obj_type, ptype, builder);
  LLVMValueRef tag = LLVMBuildExtractValue(builder, p, 0, "promise_tag");
  LLVMValueRef is_none = LLVMBuildICmp(builder, LLVMIntEQ, tag,
                                       LLVMConstInt(LLVMInt8Type(), 1, 0), "");
  LLVMBasicBlockRef is_fin_bb =
      LLVMAppendBasicBlock(loop_wrapper_fn, "inner_cor_finished");
  LLVMBasicBlockRef else_bb =
      LLVMAppendBasicBlock(loop_wrapper_fn, "inner_cor_not_finished");
  LLVMBuildCondBr(builder, is_none, is_fin_bb, else_bb);

  LLVMPositionBuilderAtEnd(builder, is_fin_bb);
  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_obj_type, inner_coro, CORO_COUNTER_SLOT, "counter_gep");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1), counter_gep);
  coro_advance(inner_coro, &tmp_ctx, builder);
  LLVMValueRef pp = coro_promise(inner_coro, coro_obj_type, ptype, builder);
  LLVMValueRef promise_gep =
      coro_promise_gep(wrapper_coro, coro_obj_type, builder);
  LLVMBuildStore(builder, pp, promise_gep);
  LLVMBuildRet(builder, wrapper_coro);

  LLVMPositionBuilderAtEnd(builder, else_bb);
  LLVMValueRef pp2 = coro_promise(inner_coro, coro_obj_type, ptype, builder);
  LLVMValueRef promise_gep2 =
      coro_promise_gep(wrapper_coro, coro_obj_type, builder);
  LLVMBuildStore(builder, pp2, promise_gep);
  LLVMBuildRet(builder, wrapper_coro);
  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef loop_coro =
      LLVMBuildMalloc(builder, coro_obj_type, "loop_wrapper_coro");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
                 LLVMBuildStructGEP2(builder, coro_obj_type, loop_coro,
                                     CORO_COUNTER_SLOT, "insert_coro_counter"));

  LLVMBuildStore(builder, loop_wrapper_fn,
                 LLVMBuildStructGEP2(builder, coro_obj_type, loop_coro,
                                     CORO_FN_PTR_SLOT, "insert_coro_fn_ptr"));

  LLVMValueRef promise_struct = LLVMGetUndef(ptype);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt8Type(), 1, 0), 0,
                                        "insert_promise_tag_none");
  LLVMBuildStore(builder, promise_struct,
                 coro_promise_gep(loop_coro, coro_obj_type, builder));

  LLVMBuildStore(builder, coro,
                 coro_state_gep(loop_coro, coro_obj_type, builder));
  return loop_coro;
}
