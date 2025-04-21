#ifndef _LANG_BACKEND_LLVM_FUNCTION_H
#define _LANG_BACKEND_LLVM_FUNCTION_H

#include "common.h"
#include "types/type.h"
#include "llvm-c/Types.h"

#define START_FUNC(_module, _name, _type)                                      \
  LLVMValueRef func = LLVMAddFunction(_module, _name, _type);                  \
  if (func == NULL) {                                                          \
    fprintf(stderr, "Error: could not create function\n");                     \
    return NULL;                                                               \
  }                                                                            \
  LLVMSetLinkage(func, LLVMExternalLinkage);                                   \
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");               \
  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(builder);                  \
  LLVMPositionBuilderAtEnd(builder, block);

#define END_FUNC LLVMPositionBuilderAtEnd(builder, prev_block);

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, TypeEnv *env,
                            LLVMModuleRef module);

LLVMValueRef codegen_extern_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder);
LLVMValueRef codegen_fn(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                        LLVMBuilderRef builder);

LLVMValueRef get_specific_callable(JITSymbol *sym, Type *expected_fn_type,
                                   JITLangCtx *ctx, LLVMModuleRef module,
                                   LLVMBuilderRef builder);

LLVMValueRef specific_fns_lookup(SpecificFns *fns, Type *key);

bool fn_types_match(Type *t1, Type *t2);

LLVMValueRef codegen_lambda_body(Ast *ast, JITLangCtx *fn_ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder);

void add_recursive_fn_ref(ObjString fn_name, LLVMValueRef func, Type *fn_type,
                          JITLangCtx *fn_ctx);

SpecificFns *specific_fns_extend(SpecificFns *fns, Type *key,
                                 LLVMValueRef func);

TypeEnv *create_env_for_generic_fn(TypeEnv *env, Type *generic_type,
                                   Type *specific_type);

void codegen_fn_type_arg_types(Type *fn_type, int fn_len,
                               LLVMTypeRef *llvm_param_types,
                               LLVMTypeRef *llvm_return_type_ref, TypeEnv *env,
                               LLVMModuleRef module);
#endif
