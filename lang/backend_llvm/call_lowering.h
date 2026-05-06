#ifndef _LANG_BACKEND_LLVM_CALL_LOWERING_H
#define _LANG_BACKEND_LLVM_CALL_LOWERING_H

#include "common.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdbool.h>

typedef struct FlatApplication {
  Ast *function;
  Ast **args;
  int num_args;
} FlatApplication;

typedef struct LoweredCallable {
  LLVMValueRef fn;
  LLVMValueRef env;
  Type *type;
  bool has_env;
} LoweredCallable;

FlatApplication flatten_application(Ast *app);
void free_flat_application(FlatApplication *app);

int callable_arg_count(Type *type);
Type *callable_arg_type(Type *type, int index);
Type *callable_return_type_after_n_args(Type *type, int num_args);
LoweredCallable lower_callable_value(LLVMValueRef callable, Type *callable_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder);
LLVMTypeRef lowered_callable_llvm_type(LoweredCallable callable,
                                       Type *original_callable_type,
                                       JITLangCtx *ctx, LLVMModuleRef module);
LLVMValueRef emit_lowered_call(Ast *app, LoweredCallable callable,
                               Type *original_callable_type, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder);

#endif
