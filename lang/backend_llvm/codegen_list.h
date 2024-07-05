#ifndef _LANG_BACKEND_LLVM_CODEGEN_LIST_H
#define _LANG_BACKEND_LLVM_CODEGEN_LIST_H

#include "common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMTypeRef list_type(Type *list_el_type);
#endif
