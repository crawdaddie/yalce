#ifndef _LANG_BACKEND_LLVM_BUILTIN_FUNCS_H
#define _LANG_BACKEND_LLVM_BUILTIN_FUNCS_H
#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

// TypeEnv *initialize_builtin_funcs(JITLangCtx *ctx, LLVMModuleRef module,
// LLVMBuilderRef builder);

LLVMValueRef _codegen_equality(Type *type, LLVMValueRef l, LLVMValueRef r,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef IndexAccessHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef create_constructor_methods(Ast *trait_impl, JITLangCtx *ctx,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef create_arithmetic_typeclass_methods(Ast *trait, JITLangCtx *ctx,
                                                 LLVMModuleRef module,
                                                 LLVMBuilderRef builder);
#endif
