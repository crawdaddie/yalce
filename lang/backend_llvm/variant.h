#ifndef _LANG_BACKEND_LLVM_VARIANT_H
#define _LANG_BACKEND_LLVM_VARIANT_H
#include "common.h"
#include "llvm-c/Types.h"

LLVMTypeRef codegen_union_type(LLVMTypeRef contained_datatypes[],
                               int variant_len, LLVMModuleRef module);

LLVMTypeRef codegen_tagged_union_type(LLVMTypeRef contained_datatypes[],
                                      int variant_len, LLVMModuleRef module);

LLVMValueRef cons_variant_member(LLVMValueRef value, int variant_idx,
                                 LLVMTypeRef union_type, LLVMModuleRef module,
                                 LLVMBuilderRef builder);

LLVMValueRef variant_extract_tag(LLVMValueRef val, LLVMBuilderRef builder);
LLVMValueRef variant_extract_value(LLVMValueRef val, LLVMTypeRef expected_type,
                                   LLVMBuilderRef builder);
LLVMValueRef match_variant_member(LLVMValueRef left, LLVMValueRef right,
                                  int variant_idx, Type *expected_member_type,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef builder);

LLVMValueRef codegen_simple_variant_member(Type *member_type,
                                           Type *variant_type);

LLVMValueRef match_simple_variant_member(Ast *id, int vidx, Type *variant_type,
                                         LLVMValueRef val, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder);

LLVMTypeRef variant_member_to_llvm_type(Type *mem_type, TypeEnv *env,
                                        LLVMModuleRef module);

LLVMValueRef tagged_union_constructor(Ast *ast, LLVMTypeRef tagged_union_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder);
#endif
