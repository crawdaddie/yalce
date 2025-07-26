#ifndef _LANG_BACKEND_LLVM_CODEGEN_TYPES_H
#define _LANG_BACKEND_LLVM_CODEGEN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"
// LLVMTypeRef type_to_llvm_type(Type *type, TypeEnv *env);
//
#define FIND_TYPE(type, env, module, ast)                                      \
  ({                                                                           \
    LLVMTypeRef _t = type_to_llvm_type(type, env, module);                     \
    if (_t == NULL) {                                                          \
      print_ast_err(ast);                                                      \
      print_location(ast);                                                     \
      print_type_err(type);                                                    \
      fprintf(stderr, "Could not find type in env");                           \
      return NULL;                                                             \
    }                                                                          \
    _t;                                                                        \
  })

LLVMTypeRef type_to_llvm_type(Type *type, JITLangCtx *ctx,
                              LLVMModuleRef module);

LLVMValueRef attempt_value_conversion(LLVMValueRef value, Type *type_from,
                                      Type *type_to, LLVMModuleRef module,
                                      LLVMBuilderRef builder);

// Define the function pointer type
LLVMValueRef double_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef ptr_constructor(LLVMValueRef val, Type *from_type,
                             LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef uint64_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder);

LLVMValueRef codegen_type_declaration(Ast *ast, JITLangCtx *ctx,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder);

LLVMValueRef codegen_eq_int(LLVMValueRef l, LLVMValueRef r,
                            LLVMModuleRef module, LLVMBuilderRef builder);

Method *get_binop_method(const char *binop, Type *l, Type *r);

TypeEnv *initialize_types(TypeEnv *env);

LLVMValueRef codegen_option_is_none(LLVMValueRef opt, LLVMBuilderRef builder);
LLVMValueRef codegen_option_is_some(LLVMValueRef opt, LLVMBuilderRef builder);

LLVMValueRef codegen_eq_num(LLVMValueRef l, LLVMValueRef r,
                            LLVMModuleRef module, LLVMBuilderRef builder);

LLVMTypeRef named_struct_type(const char *name, Type *tuple_type,
                              JITLangCtx *ctx, LLVMModuleRef module);

LLVMTypeRef type_to_llvm_type_fn_to_gen_ptr(Type *type, JITLangCtx *ctx,
                                            LLVMModuleRef module);

LLVMTypeRef codegen_option_struct_type(LLVMTypeRef type);

#ifdef __cplusplus
}
#endif
#endif
