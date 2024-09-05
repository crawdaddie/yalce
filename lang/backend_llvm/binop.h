#ifndef _LANG_BACKEND_BINOP_H
#define _LANG_BACKEND_BINOP_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_binop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_int_binop(LLVMBuilderRef builder, token_type op,
                               LLVMValueRef l, LLVMValueRef r);

LLVMValueRef codegen_float_binop(LLVMBuilderRef builder, token_type op,
                                 LLVMValueRef l, LLVMValueRef r);

typedef LLVMValueRef (*LLVMBinopMethod)(LLVMValueRef, LLVMValueRef, Type *,
                                        LLVMModuleRef, LLVMBuilderRef);
#endif
