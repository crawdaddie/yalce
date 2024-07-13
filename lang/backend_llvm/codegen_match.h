#ifndef _LANG_BACKEND_LLVM_MATCH_H
#define _LANG_BACKEND_LLVM_MATCH_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef match_values(Ast *left, LLVMValueRef right, LLVMValueRef *res,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);
#endif
