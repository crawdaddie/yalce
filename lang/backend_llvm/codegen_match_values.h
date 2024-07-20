#ifndef _LANG_BACKEND_LLVM_MATCH_VALUES_H
#define _LANG_BACKEND_LLVM_MATCH_VALUES_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMValueRef match_values(Ast *left, LLVMValueRef right, Type *right_type,
                          LLVMValueRef *res, JITLangCtx *ctx,
                          LLVMModuleRef module, LLVMBuilderRef builder);
#endif
