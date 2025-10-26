#ifndef _LANG_BACKEND_LLVM_STRINGS_H
#define _LANG_BACKEND_LLVM_STRINGS_H

#include "common.h"
#include "parse.h"
#include "llvm-c/Types.h"

LLVMValueRef llvm_string_serialize(LLVMValueRef val, Type *val_type,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder);
LLVMValueRef stream_string_concat(LLVMValueRef *strings, int num_strings,
                                  LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef insert_strlen_call(LLVMValueRef string_ptr, LLVMModuleRef module,
                                LLVMBuilderRef builder);
LLVMValueRef codegen_string(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder);

// String string_add(String a, String b);

LLVMValueRef codegen_string_add(LLVMValueRef a, LLVMValueRef b, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef StringFmtHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                              LLVMBuilderRef builder);

LLVMValueRef PrintHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder);

LLVMTypeRef string_struct_type(LLVMTypeRef data_ptr_type);

LLVMValueRef stringify_value(LLVMValueRef val, Type *val_type, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef print_str(LLVMValueRef val, JITLangCtx *ctx, LLVMModuleRef module,
                       LLVMBuilderRef builder);

#endif
