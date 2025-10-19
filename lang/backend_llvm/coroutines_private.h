#ifndef _LANG_BACKEND_LLVM___COROUTINE_H
#define _LANG_BACKEND_LLVM___COROUTINE_H
#include "common.h"
#include "parse.h"

typedef struct {
  Type *cons_type;
  LLVMTypeRef coro_obj_type;
  LLVMTypeRef promise_type;
  int num_coroutine_yields;
  int current_yield;
  AstList *yield_boundary_xs;
  int num_yield_boundary_xs;
  LLVMBasicBlockRef *branches;
  LLVMBasicBlockRef switch_default;
  LLVMValueRef switch_ref;
  LLVMValueRef func;
  LLVMTypeRef state_layout;
  LLVMBasicBlockRef cleanup_bb;
  LLVMBasicBlockRef suspend_bb;
  LLVMValueRef coro_id;   // The token from llvm.coro.id
  LLVMValueRef coro_hdl;  // The handle from llvm.coro.begin
  const char *name;
} CoroutineCtx;

LLVMValueRef coro_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMBuilderRef builder);

LLVMValueRef coro_next_set(LLVMValueRef coro, LLVMValueRef next,
                           LLVMTypeRef coro_obj_type, LLVMBuilderRef builder);

LLVMValueRef coro_next(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                       LLVMBuilderRef builder);

LLVMValueRef coro_advance(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder);

LLVMValueRef coro_replace(LLVMValueRef coro, LLVMValueRef new_coro,
                          CoroutineCtx *coro_ctx, LLVMBuilderRef builder);

LLVMValueRef coro_promise_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder);
LLVMTypeRef get_coro_state_layout(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module);

LLVMValueRef coro_state_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                            LLVMBuilderRef builder);

LLVMValueRef coro_is_finished(LLVMValueRef coro, CoroutineCtx *ctx,
                              LLVMBuilderRef builder);

LLVMValueRef coro_state(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                        LLVMBuilderRef builder);

LLVMValueRef coro_promise(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                          LLVMTypeRef promise_type, LLVMBuilderRef builder);

LLVMValueRef coro_incr(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                       LLVMBuilderRef builder);
#endif
