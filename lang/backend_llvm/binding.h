#ifndef _LANG_BACKEND_LLVM_BINDING_H
#define _LANG_BACKEND_LLVM_BINDING_H
#include "common.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_pattern_binding(Ast *binding, LLVMValueRef val,
                                     Type *val_type, JITLangCtx *ctx,
                                     LLVMModuleRef module,
                                     LLVMBuilderRef builder);

LLVMValueRef bind_value(Ast *id, LLVMValueRef val, Type *val_type,
                        JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef bind_local_value_with_storage(Ast *id, LLVMValueRef val,
                                           LLVMValueRef storage, Type *val_type,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder);
#endif
