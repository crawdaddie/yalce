#include "backend_llvm/codegen_types.h"
#include "common.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type, TypeEnv *env) {

  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef element_types[len];

  for (int i = 0; i < len; i++) {
    // Convert each element's AST node to its corresponding LLVM type
    element_types[i] = type_to_llvm_type(tuple_type->data.T_CONS.args[i], env);
  }

  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);

  return llvm_tuple_type;
}

// Function to create an LLVM list type forward decl
LLVMTypeRef list_type(Type *list_el_type, TypeEnv *env);

LLVMTypeRef type_to_llvm_type(Type *type, TypeEnv *env) {
  switch (type->kind) {

  case T_INT: {
    return LLVMInt32Type();
  }

  case T_NUM: {
    return LLVMDoubleType();
  }

  case T_BOOL: {
    return LLVMInt1Type();
  }

  case T_CHAR: {
    return LLVMInt8Type();
  }

  case T_VAR: {
    if (env) {
      return type_to_llvm_type(env_lookup(env, type->data.T_VAR), env);
    }
    return LLVMInt32Type();
  }

  case T_CONS: {
    if (strcmp(type->data.T_CONS.name, "Tuple") == 0) {
      return tuple_type(type, env);
    }

    if (strcmp(type->data.T_CONS.name, "List") == 0) {
      if (type->data.T_CONS.args[0]->kind == T_CHAR) {
        return LLVMPointerType(LLVMInt8Type(), 0);
      }

      return list_type(type->data.T_CONS.args[0], env);
    }

    return LLVMInt32Type();
  }
  case T_FN: {

    Type *fn_type = type->data.T_FN.from;
    while (fn_type->data.T_FN.to->kind == T_FN) {
      fn_type = fn_type->data.T_FN.to;
    }

    // for (int i = 0; i < fn_len; i++) {
    //   llvm_param_types[i] =
    //       type_to_llvm_type(fn_type->data.T_FN.from, ctx->env);
    //   fn_type = fn_type->data.T_FN.to;
    // }
    //
    // Type *return_type = fn_len == 0 ? fn_type->data.T_FN.to : fn_type;
    // LLVMTypeRef llvm_return_type_ref = type_to_llvm_type(return_type,
    // ctx->env);
    //
    // // Create function type with return.
    // LLVMTypeRef llvm_fn_type =
    //     LLVMFunctionType(llvm_return_type_ref, llvm_param_types, fn_len, 0);
    //
  }

  default: {
    return LLVMInt32Type();
  }
  }
}
