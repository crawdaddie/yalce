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

LLVMTypeRef codegen_tuple_field_type(int n, LLVMValueRef tuple,
                                     LLVMTypeRef tuple_type,
                                     LLVMBuilderRef builder);

LLVMValueRef codegen_tuple_to_string(LLVMValueRef opt_value, Type *val_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef codegen_tuple_gep(int n, LLVMValueRef tuple_ptr,
                               LLVMTypeRef tuple_type, LLVMBuilderRef builder);
#endif
