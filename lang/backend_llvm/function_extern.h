#ifndef _LANG_BACKEND_LLVM_FN_EXTERN_H
#define _LANG_BACKEND_LLVM_FN_EXTERN_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef get_extern_fn(const char *name, LLVMTypeRef fn_type,
                           LLVMModuleRef module);
LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef instantiate_extern_fn_sym(JITSymbol *sym, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);
#endif
