#ifndef _BACKEND_LLVM_COROUTINE_EXT_H
#define _BACKEND_LLVM_COROUTINE_EXT_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef CorLoopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef CorMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef IterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMValueRef CorOfListHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef CorOfArrayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef CorStopHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef CorCounterHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef CorStatusHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef CurrentCorHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef CorGetPromiseValHandler(Ast *ast, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef CorUnwrapOrEndHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef CorConsHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef CorGetLastValHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMBuilderRef builder);
// LLVMValueRef CorCombineHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef
// module,
//                                LLVMBuilderRef builder);
#endif
