#include "backend_llvm/codegen_types.h"
#include "types/type.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include <stdio.h>
#include <string.h>

#define LLVM_TYPE_int LLVMInt32Type()
#define LLVM_TYPE_int64 LLVMInt64Type()
#define LLVM_TYPE_bool LLVMInt1Type()
#define LLVM_TYPE_float LLVMFloatType()
#define LLVM_TYPE_double LLVMDoubleType()
#define LLVM_TYPE_void LLVMVoidType()
#define LLVM_TYPE_str LLVMPointerType(LLVMInt8Type(), 0)
#define LLVM_TYPE_char LLVMInt8Type()
#define LLVM_TYPE_ptr(type) LLVMPointerType(LLVM_TYPE_##type, 0)
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

LLVMTypeRef fn_proto_type(Type *fn_type, int fn_len, TypeEnv *env);

// Function to create an LLVM list type forward decl
LLVMTypeRef list_type(Type *list_el_type, TypeEnv *env);

LLVMTypeRef type_to_llvm_type(Type *type, TypeEnv *env) {
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
    return LLVMInt8Type();
  }

  case T_VAR: {
    if (env) {
      return type_to_llvm_type(resolve_in_env(type, env), env);
    }
    return LLVMInt32Type();
  }

  case T_CONS: {
    if (strcmp(type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0) {
      return tuple_type(type, env);
    }

    if (strcmp(type->data.T_CONS.name, TYPE_NAME_LIST) == 0) {
      if (type->data.T_CONS.args[0]->kind == T_CHAR) {
        return LLVMPointerType(LLVMInt8Type(), 0);
      }

      return list_type(type->data.T_CONS.args[0], env);
    }

    if (strcmp(type->data.T_CONS.name, TYPE_NAME_PTR) == 0) {
      return LLVM_TYPE_ptr(char);
    }

    return LLVMInt32Type();
  }

  case T_FN: {
    Type *t = type;
    int fn_len = 0;

    while (t->kind == T_FN) {
      Type *from = t->data.T_FN.from;
      t = t->data.T_FN.to;
      fn_len++;
    }
    return fn_proto_type(type, fn_len, env);
  }

  default: {
    return LLVMInt32Type();
  }
  }
}

LLVMValueRef codegen_signal_add() { return NULL; }
LLVMValueRef codegen_signal_sub() { return NULL; }
LLVMValueRef codegen_signal_mul() { return NULL; }
LLVMValueRef codegen_signal_mod() { return NULL; }

// Define the function pointer type
typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef ptr_constructor(LLVMValueRef val, Type *from_type,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {

  case T_VOID: {
    return LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0));
  }
  case T_FN: {
    printf("value conversion function to function ptr\n");
    // Create the function type
    LLVMTypeRef paramTypes[] = {
        LLVMPointerType(LLVMVoidType(), 0), // void*
        LLVMInt64Type()                     // uint64_t
    };
    LLVMTypeRef functionType =
        LLVMFunctionType(LLVMVoidType(), paramTypes, 2, 0);

    // Create a pointer to this function type
    LLVMTypeRef functionPtrType = LLVMPointerType(functionType, 0);

    // Cast the function to the correct pointer type
    return LLVMBuildBitCast(builder, val, functionPtrType, "callback_cast");
  }

  case T_CONS: {
    // printf("value conversion str to ptr\n");
    // Cast the function to the correct pointer type

    if (strcmp(from_type->data.T_CONS.name, TYPE_NAME_LIST) == 0) {
      if (from_type->data.T_CONS.args[0]->kind == T_CHAR) {
        return val;
      }
    }

    if (strcmp(from_type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0) {
      // Allocate space for the struct
      LLVMValueRef allocaInst =
          LLVMBuildAlloca(builder, LLVMTypeOf(val), "struct.addr");

      // Store the struct value into the allocated space
      LLVMBuildStore(builder, val, allocaInst);

      // The allocaInst is now a pointer to the struct
      return allocaInst;
    }
    return NULL;
  }

  default:
    return NULL;
  }
}

LLVMValueRef double_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {
  case T_NUM: {
    return val;
  }

  case T_INT: {
    return LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  }

  case T_UINT64: {
    return LLVMBuildUIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  }

  default:
    return NULL;
  }
}

LLVMValueRef uint64_constructor(LLVMValueRef val, Type *from_type,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (from_type->kind) {

  case T_INT: {
    // Get the types we need
    LLVMTypeRef i32Type = LLVM_TYPE_int;
    LLVMTypeRef i64Type = LLVM_TYPE_int64;

    // Sign extend from i32 to i64
    LLVMValueRef signExtended =
        LLVMBuildSExt(builder, val, i64Type, "signext");

    // Bitcast from signed i64 to unsigned i64
    LLVMValueRef unsignedValue =
        LLVMBuildBitCast(builder, signExtended, i64Type, "unsigned");

    return unsignedValue;
  }

  default:
    return NULL;
  }
}
void initialize_ptr_constructor() {
  t_ptr.constructor = ptr_constructor;
  t_ptr.constructor_size = sizeof(ConsMethod);
}

void initialize_double_constructor() {
  t_num.constructor = double_constructor;
  t_num.constructor_size = sizeof(ConsMethod);
}

void initialize_uint64_constructor() {
  t_uint64.constructor = uint64_constructor;
  t_uint64.constructor_size = sizeof(ConsMethod);
}

LLVMValueRef attempt_value_conversion(LLVMValueRef value, Type *type_from,
                                      Type *type_to, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  printf("attempt value conversion: ");
  if (type_from->alias) {
    printf("[%s]", type_from->alias);
  }
  print_type(type_from);
  printf(" => ");
  if (type_to->alias) {
    printf("[%s]", type_to->alias);
  }
  print_type(type_to);
  printf("\n");

  ConsMethod constructor = type_to->constructor;
  return constructor(value, type_from, module, builder);
}
