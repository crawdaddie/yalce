#ifndef _LANG_BACKEND_LLVM_CODEGEN_LIST_H
#define _LANG_BACKEND_LLVM_CODEGEN_LIST_H

#include "common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMTypeRef list_type(Type *list_el_type);

LLVMValueRef ll_get_head_val(LLVMValueRef list, LLVMTypeRef list_el_type,
                             LLVMBuilderRef builder);

LLVMValueRef ll_get_next(LLVMValueRef list, LLVMTypeRef list_el_type,
                         LLVMBuilderRef builder);

LLVMValueRef ll_is_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                        LLVMBuilderRef builder);

LLVMValueRef ll_is_not_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                            LLVMBuilderRef builder);
#endif
