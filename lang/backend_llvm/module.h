#ifndef _LANG_BACKEND_LLVM_MODULE_H
#define _LANG_BACKEND_LLVM_MODULE_H
#include "common.h"
#include "modules.h"
#include "parse.h"
#include "llvm-c/Types.h"

extern const char *module_path;
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

JITSymbol *create_module_symbol(Type *module_type, TypeEnv *module_type_env,
                                Ast *module_ast, JITLangCtx *ctx,

                                LLVMModuleRef llvm_module_ref);

LLVMValueRef compile_module(JITSymbol *module_symbol, Ast *module_ast,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder);

LLVMValueRef create_constructor_module(Ast *trait, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);
#endif
