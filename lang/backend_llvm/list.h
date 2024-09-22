#ifndef _LANG_BACKEND_LLVM_CODEGEN_LIST_H
#define _LANG_BACKEND_LLVM_CODEGEN_LIST_H

#include "backend_llvm/common.h"
#include "parse.h"
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_list(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMTypeRef list_type(Type *list_el_type, TypeEnv *env, LLVMModuleRef module);

LLVMValueRef ll_get_head_val(LLVMValueRef list, LLVMTypeRef list_el_type,
                             LLVMBuilderRef builder);

LLVMValueRef ll_get_next(LLVMValueRef list, LLVMTypeRef list_el_type,
                         LLVMBuilderRef builder);

LLVMValueRef ll_is_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                        LLVMBuilderRef builder);

LLVMValueRef ll_is_not_null(LLVMValueRef list, LLVMTypeRef list_el_type,
                            LLVMBuilderRef builder);

LLVMTypeRef llnode_type(LLVMTypeRef llvm_el_type);

LLVMValueRef null_node(LLVMTypeRef node_type);

LLVMValueRef codegen_list_prepend(LLVMValueRef l, LLVMValueRef list,
                                  JITLangCtx *ctx, LLVMModuleRef module,
                                  LLVMBuilderRef);

LLVMValueRef codegen_array(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMTypeRef codegen_array_type(Type *type, TypeEnv *env, LLVMModuleRef module);

LLVMValueRef codegen_array_at(LLVMValueRef array_ptr, LLVMValueRef idx, LLVMTypeRef el_type, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_get_array_size(LLVMBuilderRef builder, LLVMValueRef array_struct);

LLVMTypeRef create_array_struct_type(LLVMTypeRef element_type);
#endif
