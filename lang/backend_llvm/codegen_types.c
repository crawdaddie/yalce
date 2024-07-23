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

  default: {
    return LLVMInt32Type();
  }
  }
}
