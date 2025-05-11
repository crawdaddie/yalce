#ifndef _LANG_BACKEND_LLVM_CLOSURES_H
#define _LANG_BACKEND_LLVM_CLOSURES_H
#include "common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
#include <stdbool.h>
bool is_lambda_with_closures(Ast *ast);

LLVMValueRef create_curried_generic_closure_binding(
    Ast *binding, Type *closure_type, Ast *closure, JITLangCtx *ctx,
    LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef create_curried_closure_binding(Ast *binding, Type *closure_type,
                                            Ast *closure, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder);
#endif
