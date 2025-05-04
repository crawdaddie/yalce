#ifndef _LANG_BACKEND_LLVM_CODEGEN_ARRAY_H
#define _LANG_BACKEND_LLVM_CODEGEN_ARRAY_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"
LLVMValueRef codegen_create_array(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef get_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMTypeRef element_type);

LLVMValueRef set_array_element(LLVMBuilderRef builder, LLVMValueRef array,
                               LLVMValueRef index, LLVMValueRef value,
                               LLVMTypeRef element_type);
LLVMValueRef codegen_get_array_size(LLVMBuilderRef builder,
                                    LLVMValueRef array_struct,
                                    LLVMTypeRef element_type);

LLVMTypeRef codegen_array_type(LLVMTypeRef element_type);

LLVMValueRef codegen_create_array(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef ArrayFillHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef ArrayFillConstHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef ArraySuccHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef ArrayStrideHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMValueRef CStrHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder);

LLVMTypeRef tmp_generic_codegen_array_type();

LLVMValueRef ArrayOffsetHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);
LLVMValueRef ArrayRangeHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
#endif
