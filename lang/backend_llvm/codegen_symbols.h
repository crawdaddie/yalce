#ifndef _LANG_BACKEND_LLVM_SYMBOLS_H
#define _LANG_BACKEND_LLVM_SYMBOLS_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

int codegen_lookup_id(const char *id, int length, JITLangCtx *ctx,
                      JITSymbol *result);

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);
#endif
