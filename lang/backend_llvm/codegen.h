#ifndef _LANG_BACKEND_LLVM_CODEGEN_H
#define _LANG_BACKEND_LLVM_CODEGEN_H
#include "backend_llvm/common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder);
LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
#endif
