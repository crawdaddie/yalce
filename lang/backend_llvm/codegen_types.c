#include "backend_llvm/codegen_types.h"
#include "common.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type) {

  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef *element_types = malloc(len * sizeof(LLVMTypeRef));

  for (int i = 0; i < len; i++) {
    // Convert each element's AST node to its corresponding LLVM type
    element_types[i] = type_to_llvm_type(tuple_type->data.T_CONS.args[i]);
  }

  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);

  free(element_types);
  return llvm_tuple_type;
}

// Function to create an LLVM list type forward decl
LLVMTypeRef list_type(Type *list_el_type);

LLVMTypeRef type_to_llvm_type(Type *type) {
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

  case T_STRING: {
    return LLVMPointerType(LLVMInt8Type(), 0);
  }

  case T_CONS: {
    if (strcmp(type->data.T_CONS.name, "Tuple") == 0) {
      return tuple_type(type);
    }

    if (strcmp(type->data.T_CONS.name, "List") == 0) {
      return list_type(type->data.T_CONS.args[0]);
    }

    return LLVMInt32Type();
  }

  default: {
    return LLVMInt32Type();
  }
  }
}
