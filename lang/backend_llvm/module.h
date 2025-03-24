#ifndef _LANG_BACKEND_LLVM_MODULE_H
#define _LANG_BACKEND_LLVM_MODULE_H
#include "common.h"
#include "modules.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef bind_module(Ast *binding, Type *module_type,
                         LLVMValueRef module_val, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef bind_parametrized_module(Ast *binding, Ast *module_ast,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder);

LLVMValueRef bind_module_chars(const char *chars, int chars_len,
                               Type *module_type, YLCModule *mod,
                               LLVMValueRef module_val, JITLangCtx *ctx,
                               LLVMModuleRef llvm_module_ref,
                               LLVMBuilderRef builder);

LLVMValueRef codegen_module(Ast *ast, JITLangCtx *ctx,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder);
LLVMValueRef codegen_import(Ast *ast, JITLangCtx *ctx,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder);

LLVMValueRef codegen_module_access(Ast *record_ast, Type *record_type, int idx,
                                   Ast *member, Type *member_type,
                                   JITLangCtx *ctx,
                                   LLVMModuleRef llvm_module_ref,
                                   LLVMBuilderRef builder);

LLVMValueRef codegen_inline_module(Ast *binding, Ast *module_ast,
                                   JITLangCtx *ctx,
                                   LLVMModuleRef llvm_module_ref,
                                   LLVMBuilderRef builder);
#endif
