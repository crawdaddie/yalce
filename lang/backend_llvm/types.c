#include "backend_llvm/types.h"
#include "adt.h"
#include "backend_llvm/array.h"
#include "list.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LLVM_TYPE_int LLVMInt32Type()
#define LLVM_TYPE_uint64 LLVMInt64Type()
#define LLVM_TYPE_bool LLVMInt1Type()
#define LLVM_TYPE_float LLVMFloatType()
#define LLVM_TYPE_double LLVMDoubleType()
#define LLVM_TYPE_void LLVMVoidType()
#define LLVM_TYPE_str LLVMPointerType(LLVMInt8Type(), 0)
#define LLVM_TYPE_char LLVMInt8Type()
#define LLVM_TYPE_ptr(type) LLVMPointerType(LLVM_TYPE_##type, 0)

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type, TypeEnv *env, LLVMModuleRef module) {
  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef element_types[len];

  for (int i = 0; i < len; i++) {
    element_types[i] =
        type_to_llvm_type(tuple_type->data.T_CONS.args[i], env, module);
  }
  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);

  return llvm_tuple_type;
}

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, TypeEnv *env);

// Function to create an LLVM list type forward decl
LLVMTypeRef create_llvm_list_type(Type *list_el_type, TypeEnv *env,
                                  LLVMModuleRef module);

LLVMTypeRef type_to_llvm_type(Type *type, TypeEnv *env, LLVMModuleRef module) {

  // LLVMTypeRef variant = variant_member_to_llvm_type(type, env, module);
  // if (variant) {
  //   return variant;
  // }

  switch (type->kind) {

  case T_INT: {
    return LLVM_TYPE_int;
  }

  case T_NUM: {
    return LLVM_TYPE_double;
  }

  case T_BOOL: {
    return LLVM_TYPE_bool;
  }

  case T_CHAR: {
    return LLVM_TYPE_char;
  }

  case T_VAR: {
    if (env) {
      Type *lu = env_lookup(env, type->data.T_VAR);

      if (!lu) {
        fprintf(stderr, "Error type var %s not found in environment! %s:%d\n",
                type->data.T_VAR, __FILE__, __LINE__);
        return NULL;
      }

      return type_to_llvm_type(lu, env, module);
    }
    return LLVMInt32Type();
  }

  case T_TYPECLASS_RESOLVE: {
    printf("codegen tc resolve: \n");
    print_type(type);
    type = resolve_tc_rank_in_env(type, env);
    return type_to_llvm_type(type, env, module);
  }

  case T_CONS: {

    if (is_tuple_type(type)) {
      return tuple_type(type, env, module);
    }

    if (is_list_type(type)) {
      // if (type->data.T_CONS.args[0]->kind == T_CHAR) {
      //   return LLVMPointerType(LLVMInt8Type(), 0);
      // }

      return create_llvm_list_type(type->data.T_CONS.args[0], env, module);
    }

    if (is_array_type(type)) {
      return codegen_array_type(
          type_to_llvm_type(type->data.T_CONS.args[0], env, module));
    }

    if (is_pointer_type(type)) {
      return LLVM_TYPE_ptr(char);
    }

    if (type->data.T_CONS.num_args == 1) {
      return type_to_llvm_type(type->data.T_CONS.args[0], env, module);
    }

    if (strcmp(type->data.T_CONS.name, TYPE_NAME_VARIANT) == 0) {
      if (is_simple_enum(type)) {
        return LLVMInt8Type();
      } else {
        return codegen_adt_type(type, env, module);
      }
    }

    // if (type->data.T_CONS.num_args == 0) {
    //   return NULL;
    // }

    return tuple_type(type, env, module);
  }

  case T_FN: {
    Type *t = type;
    int fn_len = 0;

    while (t->kind == T_FN) {
      Type *from = t->data.T_FN.from;
      t = t->data.T_FN.to;
      fn_len++;
    }
    return codegen_fn_type(type, fn_len, env);
  }

  case T_COROUTINE_INSTANCE: {
    // Type *params_type = type->data.T_COROUTINE_INSTANCE.params_type;
    // Type *return_opt =
    //     type->data.T_COROUTINE_INSTANCE.yield_interface->data.T_FN.to;
    //
    // LLVMTypeRef fn_type = LLVMPointerType(LLVMInt8Type(), 0);
    //
    // LLVMTypeRef params_obj_type = type_to_llvm_type(
    //     type->data.T_COROUTINE_INSTANCE.params_type, env, module);
    //
    // LLVMTypeRef instance_type = coroutine_instance_type();
    // return instance_type;
    return NULL;
  }
  default: {
    return LLVMVoidType();
  }
  }

  if (is_generic(type)) {
    return NULL;
  }
}
