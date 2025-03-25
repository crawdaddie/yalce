#ifndef _LANG_BACKEND_LLVM_MODULE_H
#define _LANG_BACKEND_LLVM_MODULE_H
#include "common.h"
#include "modules.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef codegen_module(Ast *ast, JITLangCtx *ctx,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder);
JITSymbol *codegen_import(Ast *ast, Ast *binding, JITLangCtx *ctx,
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
