#ifndef _LANG_BACKEND_FUNCTION_H
#define _LANG_BACKEND_FUNCTION_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_fn_proto(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef codegen_lambda(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

JITSymbol *create_generic_fn_symbol(Ast *binding_identifier, Ast *fn_ast,
                                    JITLangCtx *ctx);
#endif
