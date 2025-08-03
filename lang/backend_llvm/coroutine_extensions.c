#include "./coroutine_extensions.h"
#include "./coroutines.h"
#include "./coroutines_private.h"
#include "array.h"
#include "function.h"
#include "list.h"
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

  ({
    LLVMValueRef pp = coro_promise(inner_coro, coro_obj_type, ptype, builder);
    LLVMValueRef promise_gep =
        coro_promise_gep(wrapper_coro, coro_obj_type, builder);
    LLVMBuildStore(builder, pp, promise_gep);

    LLVMValueRef c = LLVMConstInt(LLVMInt32Type(), 1, 1);
    LLVMBuildStore(builder, c,
                   LLVMBuildStructGEP2(builder, coro_obj_type, wrapper_coro,
                                       CORO_COUNTER_SLOT, ""));
  });

  LLVMBuildRet(builder, wrapper_coro);

  // IS NOT FIN - KEEP YIELDING AND PASSING RESULTS UP
  LLVMPositionBuilderAtEnd(builder, else_bb);
  ({
    LLVMValueRef pp = coro_promise(inner_coro, coro_obj_type, ptype, builder);
    LLVMValueRef promise_gep =
        coro_promise_gep(wrapper_coro, coro_obj_type, builder);
    LLVMBuildStore(builder, pp, promise_gep);

    LLVMValueRef cc = coro_counter(inner_coro, coro_obj_type, builder);
    LLVMBuildStore(builder, cc,
                   LLVMBuildStructGEP2(builder, coro_obj_type, wrapper_coro,
                                       CORO_COUNTER_SLOT, ""));
  });
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

LLVMValueRef new_coro_obj(LLVMValueRef func, LLVMTypeRef promise_type,
                          LLVMTypeRef coro_obj_type, LLVMValueRef state_struct,
                          LLVMTypeRef state_layout, LLVMBuilderRef builder) {

  LLVMValueRef coro = LLVMBuildMalloc(builder, coro_obj_type, "list_coro");

  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
                 LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                     CORO_COUNTER_SLOT, "insert_coro_counter"));

  LLVMBuildStore(builder, func,
                 LLVMBuildStructGEP2(builder, coro_obj_type, coro,
                                     CORO_FN_PTR_SLOT, "insert_coro_fn_ptr"));

  LLVMValueRef out_pstruct = LLVMGetUndef(promise_type);
  out_pstruct = LLVMBuildInsertValue(builder, out_pstruct,
                                     LLVMConstInt(LLVMInt8Type(), 1, 0), 0,
                                     "insert_promise_tag_none");
  LLVMBuildStore(builder, out_pstruct,
                 coro_promise_gep(coro, coro_obj_type, builder));

  LLVMValueRef state_storage =
      LLVMBuildMalloc(builder, state_layout, "state storage malloc");
  LLVMBuildStore(builder, state_struct, state_storage);

  LLVMBuildStore(builder, state_storage,
                 coro_state_gep(coro, coro_obj_type, builder));
  return coro;
}

static SpecificFns *__LIST_TO_COROUTINE_FN_CACHE = NULL;
static LLVMValueRef list_iter_func(LLVMTypeRef list_el_type,
                                   LLVMTypeRef promise_type,
                                   LLVMTypeRef coro_obj_type,
                                   LLVMTypeRef state_type, LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  LLVMValueRef func = LLVMAddFunction(module, "cor_of_list_func",
                                      PTR_ID_FUNC_TYPE(coro_obj_type));

  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef reset_bb = LLVMAppendBasicBlock(func, "reset");
  LLVMBasicBlockRef reset_merge_bb = LLVMAppendBasicBlock(func, "reset_merge");
  LLVMBasicBlockRef list_fin_block =
      LLVMAppendBasicBlock(func, "list_finished");
  LLVMBasicBlockRef list_not_fin_block =
      LLVMAppendBasicBlock(func, "list_not_finished");

  // ENTRY BLOCK
  LLVMPositionBuilderAtEnd(builder, entry_block);
  LLVMValueRef coro = LLVMGetParam(func, 0);
  LLVMValueRef counter = coro_counter(coro, coro_obj_type, builder);
  LLVMValueRef is_reset =
      LLVMBuildICmp(builder, LLVMIntEQ, counter,
                    LLVMConstInt(LLVMInt32Type(), 0, 1), "is_counter_reset?");

  // INSERT_PRINTF(1, "entry block counter: %d\n", counter);

  LLVMValueRef head_and_tail = LLVMBuildBitCast(
      builder, coro_state(coro, coro_obj_type, builder),
      LLVMPointerType(state_type, 0), "bitcast_generic_state_ptr");

  LLVMValueRef head = LLVMBuildLoad2(
      builder, LLVMPointerType(llnode_type(list_el_type), 0),
      LLVMBuildStructGEP2(builder, state_type, head_and_tail, 0, "get_head"),
      "");

  LLVMValueRef tail = LLVMBuildLoad2(
      builder, LLVMPointerType(llnode_type(list_el_type), 0),
      LLVMBuildStructGEP2(builder, state_type, head_and_tail, 1, "get_tail"),
      "");

  LLVMBuildCondBr(builder, is_reset, reset_bb, reset_merge_bb);

  // RESET (counter = 0) BLOCK
  LLVMPositionBuilderAtEnd(builder, reset_bb);

  // INSERT_PRINTF(1, "resetting coroutine with head -> tail: %d\n", counter);
  LLVMBuildStore(
      builder, LLVMConstInt(LLVMInt32Type(), 0, 1),
      LLVMBuildStructGEP2(builder, coro_obj_type, coro, CORO_COUNTER_SLOT, ""));

  LLVMBuildStore(
      builder, head,
      LLVMBuildStructGEP2(builder, state_type, head_and_tail, 1, "tail_ptr"));
  LLVMBuildStore(builder, head_and_tail,
                 coro_state_gep(coro, coro_obj_type, builder));

  LLVMBuildBr(builder, reset_merge_bb);

  // RESET MERGE (rest of entry block)
  LLVMPositionBuilderAtEnd(builder, reset_merge_bb);

  ({
    LLVMValueRef coro = LLVMGetParam(func, 0);
    LLVMValueRef head_and_tail = LLVMBuildBitCast(
        builder, coro_state(coro, coro_obj_type, builder),
        LLVMPointerType(state_type, 0), "bitcast_generic_state_ptr");

    LLVMValueRef tail = LLVMBuildLoad2(
        builder, LLVMPointerType(llnode_type(list_el_type), 0),
        LLVMBuildStructGEP2(builder, state_type, head_and_tail, 1, "get_tail"),
        "");

    LLVMValueRef tail_is_null = LLVMBuildIsNull(builder, tail, "is_tail_null");
    LLVMBuildCondBr(builder, tail_is_null, list_fin_block, list_not_fin_block);
  });

  // LIST END
  //
  LLVMPositionBuilderAtEnd(builder, list_fin_block);

  coro_terminate_block(coro,
                       &(CoroutineCtx){.coro_obj_type = coro_obj_type,
                                       .promise_type = promise_type},
                       builder);
  LLVMBuildRet(builder, coro);

  // LIST CONTINUE
  ({
    LLVMPositionBuilderAtEnd(builder, list_not_fin_block);

    LLVMValueRef coro = LLVMGetParam(func, 0);
    LLVMValueRef head_and_tail = LLVMBuildBitCast(
        builder, coro_state(coro, coro_obj_type, builder),
        LLVMPointerType(state_type, 0), "bitcast_generic_state_ptr");

    LLVMValueRef tail = LLVMBuildLoad2(
        builder, LLVMPointerType(llnode_type(list_el_type), 0),
        LLVMBuildStructGEP2(builder, state_type, head_and_tail, 1, "get_tail"),
        "");
    LLVMValueRef v = ll_get_head_val(tail, list_el_type, builder);
    coro_promise_set(coro, v, coro_obj_type, promise_type, builder);

    // INSERT_PRINTF(2, "list not null - yielding %d counter: %d\n", v,
    // counter);

    coro_incr(coro, &(CoroutineCtx){.coro_obj_type = coro_obj_type}, builder);
    LLVMValueRef incr_tail = ll_get_next(tail, list_el_type, builder);
    LLVMBuildStore(
        builder, incr_tail,
        LLVMBuildStructGEP2(builder, state_type, head_and_tail, 1, "tail_ptr"));

    LLVMBuildRet(builder, coro);
  });

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef CorOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder) {

  Type *ptype = fn_return_type(ast->md);
  Type *item_type = type_of_option(ptype);
  LLVMTypeRef list_el_type = type_to_llvm_type(item_type, ctx, module);
  LLVMValueRef func =
      specific_fns_lookup(__LIST_TO_COROUTINE_FN_CACHE, item_type);

  LLVMTypeRef promise_type = type_to_llvm_type(ptype, ctx, module);
  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

  LLVMTypeRef list_type =
      type_to_llvm_type(ast->data.AST_APPLICATION.args->md, ctx, module);

  LLVMTypeRef state_type =
      LLVMStructType((LLVMTypeRef[]){list_type, list_type}, 2, 0);

  if (!func) {
    func = list_iter_func(list_el_type, promise_type, coro_obj_type, state_type,
                          module, builder);
    __LIST_TO_COROUTINE_FN_CACHE =
        specific_fns_extend(__LIST_TO_COROUTINE_FN_CACHE, item_type, func);
  }

  LLVMValueRef list =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMValueRef state = LLVMGetUndef(state_type);
  state = LLVMBuildInsertValue(builder, state, list, 0, "insert_list_head");
  state = LLVMBuildInsertValue(builder, state, list, 1, "insert_list_head");

  LLVMValueRef list_coro = new_coro_obj(func, promise_type, coro_obj_type,
                                        state, state_type, builder);
  return list_coro;
}

static SpecificFns *__ARRAY_TO_COROUTINE_FN_CACHE = NULL;
static LLVMValueRef
array_iter_func(LLVMTypeRef arr_el_type, LLVMTypeRef promise_type,
                LLVMTypeRef coro_obj_type, LLVMTypeRef state_type,
                LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef func = LLVMAddFunction(module, "cor_of_list_func",
                                      PTR_ID_FUNC_TYPE(coro_obj_type));

  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(func, "entry");
  LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(func, "array_end");
  LLVMBasicBlockRef cont_block = LLVMAppendBasicBlock(func, "array_continue");
  LLVMPositionBuilderAtEnd(builder, entry_block);
  LLVMValueRef coro = LLVMGetParam(func, 0);
  LLVMValueRef counter = coro_counter(coro, coro_obj_type, builder);
  LLVMValueRef array = LLVMBuildLoad2(
      builder, state_type,
      LLVMBuildBitCast(builder, coro_state(coro, coro_obj_type, builder),
                       LLVMPointerType(state_type, 0),
                       "bitcast_generic_to_array"),
      "array_struct");
  LLVMValueRef array_len = codegen_get_array_size(builder, array, arr_el_type);
  LLVMValueRef is_end = LLVMBuildICmp(builder, LLVMIntEQ, counter, array_len,
                                      "counter_is_at_end");
  LLVMBuildCondBr(builder, is_end, end_block, cont_block);

  LLVMPositionBuilderAtEnd(builder, end_block);
  coro_terminate_block(coro,
                       &(CoroutineCtx){.coro_obj_type = coro_obj_type,
                                       .promise_type = promise_type},
                       builder);
  LLVMBuildRet(builder, coro);

  LLVMPositionBuilderAtEnd(builder, cont_block);
  coro_incr(coro,
            &(CoroutineCtx){.coro_obj_type = coro_obj_type,
                            .promise_type = promise_type},
            builder);
  LLVMValueRef array_item =
      get_array_element(builder, array, counter, arr_el_type);
  coro_promise_set(coro, array_item, coro_obj_type, promise_type, builder);
  LLVMBuildRet(builder, coro);

  LLVMPositionBuilderAtEnd(builder, prev_block);
  return func;
}

LLVMValueRef CorOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  Type *ptype = fn_return_type(ast->md);
  Type *item_type = type_of_option(ptype);
  LLVMTypeRef arr_el_type = type_to_llvm_type(item_type, ctx, module);
  LLVMValueRef func =
      specific_fns_lookup(__ARRAY_TO_COROUTINE_FN_CACHE, item_type);

  LLVMTypeRef promise_type = type_to_llvm_type(ptype, ctx, module);
  LLVMTypeRef coro_obj_type = CORO_OBJ_TYPE(promise_type);

  LLVMTypeRef arr_type =
      type_to_llvm_type(ast->data.AST_APPLICATION.args->md, ctx, module);

  LLVMTypeRef state_type = arr_type;

  if (!func) {
    func = array_iter_func(arr_el_type, promise_type, coro_obj_type, state_type,
                           module, builder);
    __ARRAY_TO_COROUTINE_FN_CACHE =
        specific_fns_extend(__ARRAY_TO_COROUTINE_FN_CACHE, item_type, func);
  }

  LLVMValueRef state =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);

  LLVMValueRef arr_coro = new_coro_obj(func, promise_type, coro_obj_type, state,
                                       state_type, builder);
  return arr_coro;
}

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  LLVMValueRef coro =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  coro_end_counter(coro, LLVMStructType((LLVMTypeRef[]){LLVMInt32Type()}, 1, 0),
                   builder);

  return coro;
}
