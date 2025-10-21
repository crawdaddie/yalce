#ifndef _LANG_BACKEND_LLVM_ADT
#define _LANG_BACKEND_LLVM_ADT
#include "common.h"
#include "types/type.h"
LLVMValueRef codegen_simple_enum_member(Type *enum_type, const char *mem_name,
                                        JITLangCtx *ctx, LLVMModuleRef module,
                                        LLVMBuilderRef builder);

LLVMValueRef codegen_adt_member(Type *enum_type, const char *mem_name,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder);

LLVMTypeRef codegen_adt_type(Type *type, JITLangCtx *ctx, LLVMModuleRef module);

LLVMValueRef extract_tag(LLVMValueRef val, LLVMBuilderRef builder);

LLVMValueRef codegen_some(LLVMValueRef val, LLVMBuilderRef builder);
LLVMValueRef codegen_none(LLVMBuilderRef builder);
LLVMValueRef codegen_none_typed(LLVMBuilderRef builder, LLVMTypeRef type);

LLVMValueRef opt_to_string(LLVMValueRef opt_value, Type *val_type,
                           JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef OptMapHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                           LLVMBuilderRef builder);

LLVMValueRef codegen_adt_member_with_args(Type *enum_type, LLVMTypeRef tu_type,
                                          Ast *app, const char *mem_name,
                                          JITLangCtx *ctx, LLVMModuleRef module,
                                          LLVMBuilderRef builder);

LLVMTypeRef codegen_recursive_datatype(Type *type, Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module);
#define OPTION_TAG_TYPE LLVMInt8Type()
#endif
