#ifndef _LANG_BACKEND_LLVM_COROUTINES_H
#define _LANG_BACKEND_LLVM_COROUTINES_H
#include "common.h"

LLVMValueRef compile_coroutine(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef coro_create(JITSymbol *sym, Type *expected_fn_type, Ast *app,
                         JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef coro_resume(JITSymbol *sym, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef codegen_yield(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
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
#endif
