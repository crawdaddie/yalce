#ifndef _LANG_BACKEND_LLVM_FUNCTION_H
#define _LANG_BACKEND_LLVM_FUNCTION_H
#include "common.h"
#include "types/type.h"
#include "types/typeclass.h"
#include "llvm-c/Types.h"

LLVMTypeRef fn_proto_type(Type *fn_type, int fn_len, TypeEnv *env);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);

LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef codegen_fn_application(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder);
#endif
