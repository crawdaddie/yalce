#ifndef _LANG_BACKEND_LLVM_MATCH_H
#define _LANG_BACKEND_LLVM_MATCH_H

#include "common.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_match(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef match_values(Ast *left, LLVMValueRef right, Type *right_type,
                          JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

void test_pattern_rec(Ast *pattern, BindList **bl, LLVMValueRef *test_result,
                      LLVMValueRef val, LLVMTypeRef val_type, Type *type,
                      JITLangCtx *ctx, LLVMModuleRef module,
                      LLVMBuilderRef builder);

void set_pattern_bindings(BindList *bl, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

#endif
