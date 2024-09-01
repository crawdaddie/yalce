#ifndef _LANG_BACKEND_LLVM_CODEGEN_TUPLE_H
#define _LANG_BACKEND_LLVM_CODEGEN_TUPLE_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "llvm-c/Types.h"

// Function to create an LLVM tuple value
LLVMValueRef codegen_tuple(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_tuple_access(int n, LLVMValueRef tuple,
                                  LLVMTypeRef tuple_type,
                                  LLVMBuilderRef builder);
#endif
