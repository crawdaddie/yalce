#ifndef _LANG_BACKEND_LLVM_MODULE_H
#define _LANG_BACKEND_LLVM_MODULE_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_module(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef bind_module(Ast *binding, Type *module_type,
                         LLVMValueRef module_val, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef bind_parametrized_module(Ast *binding, Ast *module_ast,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder);
#endif
