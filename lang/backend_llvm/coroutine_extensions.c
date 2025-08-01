#include "./coroutine_extensions.h"
#include "./coroutines.h"
#include "./coroutines_private.h"
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

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_block =
      LLVMAppendBasicBlock(loop_wrapper_fn, "entry");
  LLVMBasicBlockRef is_fin_bb =
      LLVMAppendBasicBlock(loop_wrapper_fn, "inner_cor_finished");
  LLVMBasicBlockRef else_bb =
      LLVMAppendBasicBlock(loop_wrapper_fn, "inner_cor_not_finished");

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
  LLVMBuildCondBr(builder, is_none, is_fin_bb, else_bb);

  // IS FIN -> LOOP BACK AROUND
  LLVMPositionBuilderAtEnd(builder, is_fin_bb);
  LLVMValueRef counter_gep = LLVMBuildStructGEP2(
      builder, coro_obj_type, inner_coro, CORO_COUNTER_SLOT, "counter_gep");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1), counter_gep);
  // reset inner cor counter
  coro_advance(inner_coro, &tmp_ctx, builder);

  LLVMValueRef pp = coro_promise(inner_coro, coro_obj_type, ptype, builder);
  LLVMValueRef promise_gep =
      coro_promise_gep(wrapper_coro, coro_obj_type, builder);
  LLVMBuildStore(builder, pp, promise_gep);

  LLVMValueRef c = LLVMConstInt(LLVMInt32Type(), 1, 1);
  LLVMBuildStore(builder, c,
                 LLVMBuildStructGEP2(builder, coro_obj_type, wrapper_coro,
                                     CORO_COUNTER_SLOT, ""));

  LLVMBuildRet(builder, wrapper_coro);

  // IS NOT FIN - KEEP YIELDING AND PASSING RESULTS UP
  LLVMPositionBuilderAtEnd(builder, else_bb);
  LLVMValueRef pp2 = coro_promise(inner_coro, coro_obj_type, ptype, builder);
  LLVMValueRef promise_gep2 =
      coro_promise_gep(wrapper_coro, coro_obj_type, builder);
  LLVMBuildStore(builder, pp2, promise_gep);

  LLVMValueRef cc = coro_counter(inner_coro, coro_obj_type, builder);
  LLVMBuildStore(builder, cc,
                 LLVMBuildStructGEP2(builder, coro_obj_type, wrapper_coro,
                                     CORO_COUNTER_SLOT, ""));
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
LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder) {
  Type *out_cor_type = ast->md;
  Type *out_ptype = fn_return_type(out_cor_type);
  LLVMTypeRef out_promise_type = type_to_llvm_type(out_ptype, ctx, module);
  LLVMTypeRef out_cor_obj_type = CORO_OBJ_TYPE(out_promise_type);

  Type *in_cor_type = (ast->data.AST_APPLICATION.args + 1)->md;
  Type *in_ptype = fn_return_type(in_cor_type);
  LLVMTypeRef in_promise_type = type_to_llvm_type(in_ptype, ctx, module);
  LLVMTypeRef in_cor_obj_type = CORO_OBJ_TYPE(in_promise_type);

  Type map_type;
  map_type = *((Type *)ast->data.AST_APPLICATION.args->md);
  if (is_generic(&map_type)) {
    Type *f = fn_return_type(in_cor_type);
    f = type_of_option(f);
    Type *t = fn_return_type(out_cor_type);
    t = type_of_option(t);
    map_type = (Type){T_FN, {.T_FN = {.from = f, .to = t}}};
    ast->data.AST_APPLICATION.args->md = &map_type;
  }
  // print_type(&map_type);

  LLVMValueRef coro =
      codegen(ast->data.AST_APPLICATION.args + 1, ctx, module, builder);
  LLVMValueRef map_func =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMValueRef map_cor_fn = LLVMAddFunction(module, "map_coroutine_fn",
                                            PTR_ID_FUNC_TYPE(out_cor_obj_type));
  LLVMSetLinkage(map_cor_fn, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);

  LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(map_cor_fn, "entry");
  LLVMBasicBlockRef reset_bb =
      LLVMAppendBasicBlock(map_cor_fn, "reset_children");
  LLVMBasicBlockRef reset_merge_bb =
      LLVMAppendBasicBlock(map_cor_fn, "merge_after_reset");
  LLVMBasicBlockRef is_fin_bb =
      LLVMAppendBasicBlock(map_cor_fn, "inner_cor_finished");
  LLVMBasicBlockRef else_bb =
      LLVMAppendBasicBlock(map_cor_fn, "inner_cor_not_finished");

  LLVMPositionBuilderAtEnd(builder, entry_block);

  LLVMValueRef wrapper_coro = LLVMGetParam(map_cor_fn, 0);

  LLVMValueRef inner_coro = LLVMBuildBitCast(
      builder, coro_state(wrapper_coro, in_cor_obj_type, builder),
      LLVMPointerType(in_cor_obj_type, 0), "bitcast_generic_state_ptr");

  LLVMValueRef counter = coro_counter(wrapper_coro, out_cor_obj_type, builder);
  LLVMValueRef is_reset =
      LLVMBuildICmp(builder, LLVMIntEQ, counter,
                    LLVMConstInt(LLVMInt32Type(), 0, 1), "is_counter_reset?");

  LLVMBuildCondBr(builder, is_reset, reset_bb, reset_merge_bb);
  LLVMPositionBuilderAtEnd(builder, reset_bb);
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
                 LLVMBuildStructGEP2(builder, in_cor_obj_type, inner_coro,
                                     CORO_COUNTER_SLOT, ""));
  LLVMBuildBr(builder, reset_merge_bb);
  LLVMPositionBuilderAtEnd(builder, reset_merge_bb);

  coro_advance(inner_coro, &(CoroutineCtx){.coro_obj_type = in_cor_obj_type},
               builder);
  coro_incr(wrapper_coro, &(CoroutineCtx){.coro_obj_type = out_cor_obj_type},
            builder);

  LLVMValueRef p =
      coro_promise(inner_coro, in_cor_obj_type, in_promise_type, builder);
  LLVMValueRef tag = LLVMBuildExtractValue(builder, p, 0, "promise_tag");
  LLVMValueRef is_none = LLVMBuildICmp(builder, LLVMIntEQ, tag,
                                       LLVMConstInt(LLVMInt8Type(), 1, 0), "");
  LLVMBuildCondBr(builder, is_none, is_fin_bb, else_bb);

  // IS FINISHED BLOCK
  LLVMPositionBuilderAtEnd(builder, is_fin_bb);

  LLVMValueRef promise_struct = LLVMGetUndef(out_promise_type);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt32Type(), 1, 0), 0,
                                        "insert_promise_tag_none");
  LLVMBuildStore(builder, promise_struct,
                 coro_promise_gep(wrapper_coro, out_cor_obj_type, builder));

  // LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), -1, 1),
  //                LLVMBuildStructGEP2(builder, out_cor_obj_type, wrapper_coro,
  //                                    CORO_COUNTER_SLOT, ""));
  LLVMBuildRet(builder, wrapper_coro);

  // IS NOT FINISHED BLOCK
  LLVMPositionBuilderAtEnd(builder, else_bb);
  LLVMValueRef pval =
      LLVMBuildExtractValue(builder, p, 1, "extract_val_from_prom");
  LLVMValueRef mapped_pval =
      LLVMBuildCall2(builder, type_to_llvm_type(&map_type, ctx, module),
                     map_func, (LLVMValueRef[]){pval}, 1, "call_map_func");

  promise_struct = LLVMGetUndef(out_promise_type);
  promise_struct = LLVMBuildInsertValue(builder, promise_struct,
                                        LLVMConstInt(LLVMInt32Type(), 0, 0), 0,
                                        "insert_promise_tag_none");

  promise_struct = LLVMBuildInsertValue(builder, promise_struct, mapped_pval, 1,
                                        "insert_promise_val");
  LLVMBuildStore(builder, promise_struct,
                 coro_promise_gep(wrapper_coro, out_cor_obj_type, builder));
  LLVMBuildRet(builder, wrapper_coro);

  LLVMPositionBuilderAtEnd(builder, prev_block);

  LLVMValueRef mapped_coro =
      LLVMBuildMalloc(builder, out_cor_obj_type, "map_wrapper_coro");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
                 LLVMBuildStructGEP2(builder, out_cor_obj_type, mapped_coro,
                                     CORO_COUNTER_SLOT, "insert_coro_counter"));

  LLVMBuildStore(builder, map_cor_fn,
                 LLVMBuildStructGEP2(builder, out_cor_obj_type, mapped_coro,
                                     CORO_FN_PTR_SLOT, "insert_coro_fn_ptr"));

  LLVMValueRef out_pstruct = LLVMGetUndef(out_promise_type);
  out_pstruct = LLVMBuildInsertValue(builder, out_pstruct,
                                     LLVMConstInt(LLVMInt8Type(), 1, 0), 0,
                                     "insert_promise_tag_none");
  LLVMBuildStore(builder, out_pstruct,
                 coro_promise_gep(mapped_coro, out_cor_obj_type, builder));

  LLVMBuildStore(builder, coro,
                 coro_state_gep(mapped_coro, out_cor_obj_type, builder));
  return mapped_coro;
}

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  printf("iter handler\n");
  print_ast(ast);
  return NULL;
}
