#ifndef _LANG_BACKEND_LLVM_APPLICATION_H
#define _LANG_BACKEND_LLVM_APPLICATION_H

#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_application(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef handle_type_conversions(LLVMValueRef val, Type *from_type,
                                     Type *to_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef call_callable(Ast *ast, Type *callable_type, LLVMValueRef callable,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);
#endif
