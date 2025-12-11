#ifndef _LANG_BACKEND_LLVM_COROUTINES_H
#define _LANG_BACKEND_LLVM_COROUTINES_H
#include "../common.h"
#include "coroutines_private.h"

LLVMValueRef compile_coroutine(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef coro_promise_set(LLVMValueRef coro, LLVMValueRef val,
                              LLVMTypeRef coro_obj_type,
                              LLVMTypeRef promise_type, LLVMBuilderRef builder);

LLVMValueRef coro_counter_gep(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder);

LLVMValueRef coro_end_counter(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                              LLVMBuilderRef builder);

LLVMValueRef coro_promise_set_none(LLVMValueRef coro, LLVMTypeRef coro_obj_type,
                                   LLVMTypeRef promise_type,
                                   LLVMBuilderRef builder);

void coro_terminate_block(LLVMValueRef coro, CoroutineCtx *coro_ctx,
                          LLVMBuilderRef builder);

LLVMValueRef coro_jump_to_next_block(LLVMValueRef coro, LLVMValueRef next_coro,
                                     CoroutineCtx *coro_ctx,
                                     LLVMBuilderRef builder);
#define CORO_COUNTER_SLOT 0
#define CORO_FN_PTR_SLOT 1
#define CORO_STATE_SLOT 2
#define CORO_NEXT_SLOT 3
#define CORO_PROMISE_SLOT 4

#define CORO_OBJ_TYPE(ptype)                                                   \
  LLVMStructType((LLVMTypeRef[]){                                              \
                     /* counter: */ LLVMInt32Type(),                           \
                     /* fn_ptr: */ GENERIC_PTR,                                \
                     /* state: */ GENERIC_PTR,                                 \
                     /* next: */ GENERIC_PTR,                                  \
                     /* promise: */ ptype,                                     \
                 },                                                            \
                 5, 0)

#define PTR_ID_FUNC_TYPE(obj)                                                  \
  LLVMFunctionType(LLVMPointerType(obj, 0),                                    \
                   (LLVMTypeRef[]){LLVMPointerType(obj, 0)}, 1, 0)
#define COR_END_KW "cor_end"

LLVMValueRef CorGetLastValHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);
LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);
LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);
LLVMValueRef CorOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);
LLVMValueRef CorOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef PlayRoutineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);
LLVMValueRef CurrentCorHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef CorUnwrapOrEndHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef create_coroutine_symbol(Ast *binding, Ast *expr, Type *expr_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);
#endif
