#include "backend_llvm/types.h"
#include "adt.h"
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

typedef struct StructCache {
  Type *type;
  LLVMTypeRef struct_type;
  struct StructCache *next;
} StructCache;

static StructCache *struct_cache = NULL;

StructCache *struct_cache_extend(StructCache *env, Type *type,
                                 LLVMTypeRef struct_type) {
  StructCache *new_env = malloc(sizeof(StructCache));
  new_env->type = type;
  new_env->struct_type = struct_type;
  new_env->next = env;
  return new_env;
}

LLVMTypeRef struct_cache_lookup(StructCache *env, Type *type) {
  while (env) {
    if (types_equal(type, env->type)) {
      return env->struct_type;
    }
    env = env->next;
  }
  return NULL;
}

// Function to create an LLVM tuple type
LLVMTypeRef tuple_type(Type *tuple_type, TypeEnv *env, LLVMModuleRef module) {
  // LLVMTypeRef cached_struct = struct_cache_lookup(struct_cache, tuple_type);
  // if (cached_struct) {
  //   return cached_struct;
  // }

  int len = tuple_type->data.T_CONS.num_args;

  LLVMTypeRef element_types[len];

  for (int i = 0; i < len; i++) {
    element_types[i] =
        type_to_llvm_type(tuple_type->data.T_CONS.args[i], env, module);
  }
  // printf("segfault???\n");
  // LLVMContextRef ctx_ref = LLVMGetModuleContext(module);
  //
  // LLVMTypeRef llvm_tuple_type =
  //     LLVMStructTypeInContext(ctx_ref, element_types, len, 0);
  //
  LLVMTypeRef llvm_tuple_type = LLVMStructType(element_types, len, 0);

  // LLVMTypeRef llvm_tuple_type = LLVMStructCreateNamed(ctx_ref, "");
  // LLVMStructSetBody(llvm_tuple_type, element_types, len, 0);

  // struct_cache = struct_cache_extend(struct_cache, tuple_type,
  // llvm_tuple_type);

  return llvm_tuple_type;
}

LLVMTypeRef codegen_fn_type(Type *fn_type, int fn_len, TypeEnv *env);

// Function to create an LLVM list type forward decl
LLVMTypeRef create_list_type(Type *list_el_type, TypeEnv *env,
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
    if (!is_generic(type)) {
      type = resolve_tc_rank(type);
      return type_to_llvm_type(type, env, module);
    }
    return NULL;
  }

  case T_CONS: {

    if (is_tuple_type(type)) {
      return tuple_type(type, env, module);
    }

    if (is_list_type(type)) {
      // if (type->data.T_CONS.args[0]->kind == T_CHAR) {
      //   return LLVMPointerType(LLVMInt8Type(), 0);
      // }

      return create_list_type(type->data.T_CONS.args[0], env, module);
    }

    if (is_array_type(type)) {
      return codegen_array_type(type->data.T_CONS.args[0], env, module);
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

LLVMValueRef codegen_signal_add() { return NULL; }
LLVMValueRef codegen_signal_sub() { return NULL; }
LLVMValueRef codegen_signal_mul() { return NULL; }
LLVMValueRef codegen_signal_mod() { return NULL; }
/*
LLVMValueRef ptr_constructor(LLVMValueRef val, Type *from_type,
                             LLVMModuleRef module, LLVMBuilderRef builder) {

  if (is_string_type(from_type)) {
    LLVMTypeRef el_type = LLVMTypeOf(val);
    LLVMTypeRef pointer_type = LLVMPointerType(el_type, 0);
    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0)};
    LLVMValueRef ptr =
        LLVMBuildGEP2(builder, el_type, val, indices, 1, "addr_of");

    return ptr;
  }
  switch (from_type->kind) {

  case T_VOID: {
    return LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0));
  }
  case T_FN: {
    // printf("ptr cons\n");
    // printf("value conversion function to function ptr\n");
    // LLVMTypeRef paramTypes[] = {
    //     LLVMPointerType(LLVMVoidType(), 0), // void*
    //     LLVMInt64Type()                     // uint64_t
    // };
    // LLVMTypeRef functionType =
    //     LLVMFunctionType(LLVMVoidType(), paramTypes, 2, 0);
    //
    // LLVMTypeRef functionPtrType = LLVMPointerType(functionType, 0);
    //
    // return LLVMBuildBitCast(builder, val, functionPtrType, "callback_cast");
    return val;
  }

  // printf("ptr cons\n");
  case T_CONS: {
    if (is_tuple_type(from_type)) {
      LLVMTypeRef structType = LLVMTypeOf(val);

      // Allocate space for the struct on the stack
      LLVMValueRef allocaInst =
          LLVMBuildMalloc(builder, structType, "struct.ptr");

      // Store the struct value into the allocated space
      LLVMBuildStore(builder, val, allocaInst);

      // allocaInst is now a pointer to the address of the struct
      return allocaInst;
    }

    LLVMTypeRef el_type = LLVMTypeOf(val);
    LLVMTypeRef pointer_type = LLVMPointerType(el_type, 0);
    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0)};
    LLVMValueRef ptr =
        LLVMBuildGEP2(builder, el_type, val, indices, 1, "addr_of");

    // LLVMDumpValue(ptr);
    // LLVMDumpType(LLVMTypeOf(ptr));
    // printf("\n");
    return ptr;

    // return val;

    // return val;
  }

  default: {

    LLVMTypeRef el_type = LLVMTypeOf(val);
    LLVMTypeRef pointer_type = LLVMPointerType(el_type, 0);
    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0)};
    LLVMValueRef ptr =
        LLVMBuildGEP2(builder, el_type, val, indices, 1, "addr_of");

    // LLVMDumpValue(ptr);
    // LLVMDumpType(LLVMTypeOf(ptr));
    // printf("\n");
    return ptr;
  }
  }
}
*/

typedef LLVMValueRef (*ConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                   LLVMBuilderRef);

LLVMValueRef attempt_value_conversion(LLVMValueRef value, Type *type_from,
                                      Type *type_to, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  if (!type_to->constructor) {
    return value;
  }
  ConsMethod constructor = type_to->constructor;
  return constructor(value, type_from, module, builder);
}
// clang-format off
static int int_ops_map[] = {
  [TOKEN_PLUS] =      LLVMAdd,
  [TOKEN_MINUS] =     LLVMSub,
  [TOKEN_STAR] =      LLVMMul,
  [TOKEN_SLASH] =     LLVMSDiv,
  [TOKEN_MODULO] =    LLVMSRem,

  [TOKEN_LT] =        LLVMIntSLT,
  [TOKEN_LTE] =       LLVMIntSLE,
  [TOKEN_GT] =        LLVMIntSGT,
  [TOKEN_GTE] =       LLVMIntSGE,
  [TOKEN_EQUALITY] =  LLVMIntEQ,
  [TOKEN_NOT_EQUAL] = LLVMIntNE,
};

static int float_ops_map[] = {
  [TOKEN_PLUS] =      LLVMFAdd,
  [TOKEN_MINUS] =     LLVMFSub,
  [TOKEN_STAR] =      LLVMFMul,   
  [TOKEN_SLASH] =     LLVMFDiv,
  [TOKEN_MODULO] =    LLVMFRem,

  [TOKEN_LT] =        LLVMRealOLT,
  [TOKEN_LTE] =       LLVMRealOLE,
  [TOKEN_GT] =        LLVMRealOGT,
  [TOKEN_GTE] =       LLVMRealOGE,
  [TOKEN_EQUALITY] =  LLVMRealOEQ,
  [TOKEN_NOT_EQUAL] = LLVMRealONE,

};

// clang-format on

LLVMValueRef codegen_int_binop(LLVMBuilderRef builder, token_type op,
                               LLVMValueRef l, LLVMValueRef r) {

  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    return LLVMBuildBinOp(builder, int_ops_map[op], l, r, "");
  }
  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL: {
    return LLVMBuildICmp(builder, int_ops_map[op], l, r, "");
  }
  default:
    return NULL;
  }
}

LLVMValueRef codegen_float_binop(LLVMBuilderRef builder, token_type op,
                                 LLVMValueRef l, LLVMValueRef r) {
  switch (op) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_MODULO: {
    return LLVMBuildBinOp(builder, float_ops_map[op], l, r, "float_binop");
  }

  case TOKEN_LT:
  case TOKEN_LTE:
  case TOKEN_GT:
  case TOKEN_GTE:
  case TOKEN_EQUALITY:
  case TOKEN_NOT_EQUAL: {
    return LLVMBuildFCmp(builder, float_ops_map[op], l, r, "");
  }
  }
}

typedef LLVMValueRef (*EqMethod)(LLVMValueRef, LLVMValueRef, LLVMModuleRef,
                                 LLVMBuilderRef);

LLVMValueRef codegen_eq_int(LLVMValueRef l, LLVMValueRef r,
                            LLVMModuleRef module, LLVMBuilderRef builder) {
  return LLVMBuildICmp(builder, LLVMIntEQ, l, r, "Int ==");
}

static LLVMValueRef codegen_neq_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntNE, l, r, "Int !=");
}

static LLVMValueRef codegen_eq_uint64(LLVMValueRef l, LLVMValueRef r,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntEQ, l, r, "Uint64 ==");
}
static LLVMValueRef codegen_neq_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntNE, l, r, "Uint64 !=");
}

LLVMValueRef codegen_eq_num(LLVMValueRef l, LLVMValueRef r,
                            LLVMModuleRef module, LLVMBuilderRef builder) {

  return LLVMBuildFCmp(builder, LLVMRealOEQ, l, r, "Num ==");
}
static LLVMValueRef codegen_neq_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildFCmp(builder, LLVMRealONE, l, r, "Num !=");
}

typedef LLVMValueRef (*OrdMethod)(LLVMValueRef, LLVMValueRef, LLVMModuleRef,
                                  LLVMBuilderRef);
static LLVMValueRef codegen_lt_int(LLVMValueRef l, LLVMValueRef r,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return LLVMBuildICmp(builder, LLVMIntSLT, l, r, "Int <");
}
static LLVMValueRef codegen_gt_int(LLVMValueRef l, LLVMValueRef r,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntSGT, l, r, "Int >");
}
static LLVMValueRef codegen_lte_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntSLE, l, r, "Int <=");
}
static LLVMValueRef codegen_gte_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntSGE, l, r, "Int >=");
}

static LLVMValueRef codegen_lt_uint64(LLVMValueRef l, LLVMValueRef r,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntULT, l, r, "Uint64 <");
}
static LLVMValueRef codegen_gt_uint64(LLVMValueRef l, LLVMValueRef r,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntUGT, l, r, "Uint64 >");
}
static LLVMValueRef codegen_lte_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntULE, l, r, "Uint64 <=");
}
static LLVMValueRef codegen_gte_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  return LLVMBuildICmp(builder, LLVMIntUGE, l, r, "Uint64 >=");
}

static LLVMValueRef codegen_lt_num(LLVMValueRef l, LLVMValueRef r,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  return LLVMBuildFCmp(builder, LLVMRealOLT, l, r, "Num <");
}
static LLVMValueRef codegen_gt_num(LLVMValueRef l, LLVMValueRef r,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return LLVMBuildFCmp(builder, LLVMRealOGT, l, r, "Num >");
}
static LLVMValueRef codegen_lte_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildFCmp(builder, LLVMRealOLE, l, r, "Num <=");
}
static LLVMValueRef codegen_gte_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildFCmp(builder, LLVMRealOGE, l, r, "Num >=");
}

typedef LLVMValueRef (*ArithmeticMethod)(LLVMValueRef, LLVMValueRef,
                                         LLVMModuleRef, LLVMBuilderRef);
static LLVMValueRef codegen_add_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMAdd, l, r, "Int +");
}
static LLVMValueRef codegen_sub_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMSub, l, r, "Int -");
}
static LLVMValueRef codegen_mul_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMMul, l, r, "Int *");
}
static LLVMValueRef codegen_div_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMSDiv, l, r, "Int /");
}
static LLVMValueRef codegen_mod_int(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMSRem, l, r, "Int %");
}

static LLVMValueRef codegen_add_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMAdd, l, r, "Uint64 +");
}
static LLVMValueRef codegen_sub_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMSub, l, r, "Uint64 -");
}
static LLVMValueRef codegen_mul_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMMul, l, r, "Uint64 *");
}
static LLVMValueRef codegen_div_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMUDiv, l, r, "Uint64 /");
}
static LLVMValueRef codegen_mod_uint64(LLVMValueRef l, LLVMValueRef r,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMURem, l, r, "Uint64 %");
}

static LLVMValueRef codegen_add_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMFAdd, l, r, "Num *");
}
static LLVMValueRef codegen_sub_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildBinOp(builder, LLVMFSub, l, r, "Num -");
}
static LLVMValueRef codegen_mul_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMFMul, l, r, "Num *");
}
static LLVMValueRef codegen_div_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {

  return LLVMBuildBinOp(builder, LLVMFDiv, l, r, "Num /");
}
static LLVMValueRef codegen_mod_num(LLVMValueRef l, LLVMValueRef r,
                                    LLVMModuleRef module,
                                    LLVMBuilderRef builder) {
  return LLVMBuildBinOp(builder, LLVMFRem, l, r, "Num %");
}

#define EQ_TC(t) t.implements[0]
#define ORD_TC(t) t.implements[1]
#define ARITHMETIC_TC(t) t.implements[2]

void initialize_builtin_numeric_types(TypeEnv *env) {
  // EQ_TC(t_int)->methods[0].method = &codegen_eq_int;
  // EQ_TC(t_int)->methods[0].size = sizeof(EqMethod);
  // EQ_TC(t_int)->methods[1].method = &codegen_neq_int;
  // EQ_TC(t_int)->methods[1].size = sizeof(EqMethod);
  //
  // EQ_TC(t_bool)->methods[0].method = &codegen_eq_int;
  // EQ_TC(t_bool)->methods[0].size = sizeof(EqMethod);
  // EQ_TC(t_bool)->methods[1].method = &codegen_neq_int;
  // EQ_TC(t_bool)->methods[1].size = sizeof(EqMethod);
  //
  // ORD_TC(t_int)->methods[0].method = &codegen_lt_int;
  // ORD_TC(t_int)->methods[0].size = sizeof(OrdMethod);
  // ORD_TC(t_int)->methods[1].method = &codegen_gt_int;
  // ORD_TC(t_int)->methods[1].size = sizeof(OrdMethod);
  // ORD_TC(t_int)->methods[2].method = &codegen_lte_int;
  // ORD_TC(t_int)->methods[2].size = sizeof(OrdMethod);
  // ORD_TC(t_int)->methods[3].method = &codegen_gte_int;
  // ORD_TC(t_int)->methods[3].size = sizeof(OrdMethod);
  //
  // ARITHMETIC_TC(t_int)->methods[0].method = &codegen_add_int;
  // ARITHMETIC_TC(t_int)->methods[0].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_int)->methods[1].method = &codegen_sub_int;
  // ARITHMETIC_TC(t_int)->methods[1].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_int)->methods[2].method = &codegen_mul_int;
  // ARITHMETIC_TC(t_int)->methods[2].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_int)->methods[3].method = &codegen_div_int;
  // ARITHMETIC_TC(t_int)->methods[3].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_int)->methods[4].method = &codegen_mod_int;
  // ARITHMETIC_TC(t_int)->methods[4].size = sizeof(ArithmeticMethod);
  //
  // EQ_TC(t_uint64)->methods[0].method = &codegen_eq_uint64;
  // EQ_TC(t_uint64)->methods[0].size = sizeof(EqMethod);
  // EQ_TC(t_uint64)->methods[1].method = &codegen_neq_uint64;
  // EQ_TC(t_uint64)->methods[1].size = sizeof(EqMethod);
  //
  // ORD_TC(t_uint64)->methods[0].method = &codegen_lt_uint64;
  // ORD_TC(t_uint64)->methods[0].size = sizeof(OrdMethod);
  // ORD_TC(t_uint64)->methods[1].method = &codegen_gt_uint64;
  // ORD_TC(t_uint64)->methods[1].size = sizeof(OrdMethod);
  // ORD_TC(t_uint64)->methods[2].method = &codegen_lte_uint64;
  // ORD_TC(t_uint64)->methods[2].size = sizeof(OrdMethod);
  // ORD_TC(t_uint64)->methods[3].method = &codegen_gte_uint64;
  // ORD_TC(t_uint64)->methods[3].size = sizeof(OrdMethod);
  //
  // ARITHMETIC_TC(t_uint64)->methods[0].method = &codegen_add_uint64;
  // ARITHMETIC_TC(t_uint64)->methods[0].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_uint64)->methods[1].method = &codegen_sub_uint64;
  // ARITHMETIC_TC(t_uint64)->methods[1].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_uint64)->methods[2].method = &codegen_mul_uint64;
  // ARITHMETIC_TC(t_uint64)->methods[2].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_uint64)->methods[3].method = &codegen_div_uint64;
  // ARITHMETIC_TC(t_uint64)->methods[3].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_uint64)->methods[4].method = &codegen_mod_uint64;
  // ARITHMETIC_TC(t_uint64)->methods[4].size = sizeof(ArithmeticMethod);

  t_uint64.constructor = uint64_constructor;
  t_uint64.constructor_size = sizeof(ConsMethod);

  // EQ_TC(t_num)->methods[0].method = &codegen_eq_num;
  // EQ_TC(t_num)->methods[0].size = sizeof(EqMethod);
  // EQ_TC(t_num)->methods[1].method = &codegen_neq_num;
  // EQ_TC(t_num)->methods[1].size = sizeof(EqMethod);

  // ORD_TC(t_num)->methods[0].method = &codegen_lt_num;
  // ORD_TC(t_num)->methods[0].size = sizeof(OrdMethod);
  // ORD_TC(t_num)->methods[1].method = &codegen_gt_num;
  // ORD_TC(t_num)->methods[1].size = sizeof(OrdMethod);
  // ORD_TC(t_num)->methods[2].method = &codegen_lte_num;
  // ORD_TC(t_num)->methods[2].size = sizeof(OrdMethod);
  // ORD_TC(t_num)->methods[3].method = &codegen_gte_num;
  // ORD_TC(t_num)->methods[3].size = sizeof(OrdMethod);

  // ARITHMETIC_TC(t_num)->methods[0].method = &codegen_add_num;
  // ARITHMETIC_TC(t_num)->methods[0].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_num)->methods[1].method = &codegen_sub_num;
  // ARITHMETIC_TC(t_num)->methods[1].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_num)->methods[2].method = &codegen_mul_num;
  // ARITHMETIC_TC(t_num)->methods[2].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_num)->methods[3].method = &codegen_div_num;
  // ARITHMETIC_TC(t_num)->methods[3].size = sizeof(ArithmeticMethod);
  // ARITHMETIC_TC(t_num)->methods[4].method = &codegen_mod_num;
  // ARITHMETIC_TC(t_num)->methods[4].size = sizeof(ArithmeticMethod);

  t_num.constructor = double_constructor;
  t_num.constructor_size = sizeof(ConsMethod);
}

typedef struct _tc_key {
  const char *binop;
  int tc_idx;
  int meth_idx;
} _tc_key;

// clang-format off
static _tc_key tc_keys[] = {
    {"==", 0, 0},
    {"!=", 0, 1},
    {"<", 1, 0},
    {">", 1, 1},
    {"<=", 1, 2},
    {">=", 1, 3},
    {"+", 2, 0},
    {"-", 2, 1},
    {"*", 2, 2},
    {"/", 2, 3},
    {"%", 2, 4},
    // {"::", 2, 4},
};

// clang-format on

#define _BINOP_METHOD_CHECKS

Method *get_binop_method(const char *binop, Type *l, Type *r) {
  printf("get binop method for %s\n", binop);
  print_type(l);
  print_type(r);

  //   if (l == NULL) {
  //     for (int i = 0; i < 11; i++) {
  //       _tc_key tc_key = tc_keys[i];
  //       return r->implements[tc_key.tc_idx]->methods + tc_key.meth_idx;
  //     }
  //   }
  //
  //   if (r == NULL) {
  //     for (int i = 0; i < 11; i++) {
  //       _tc_key tc_key = tc_keys[i];
  //       return l->implements[tc_key.tc_idx]->methods + tc_key.meth_idx;
  //     }
  //   }
  //
  //   for (int i = 0; i < 11; i++) {
  //     _tc_key tc_key = tc_keys[i];
  //     if (strcmp(binop, tc_key.binop) == 0) {
  //
  // #ifdef _BINOP_METHOD_CHECKS
  //       if (tc_key.tc_idx >= l->num_implements ||
  //           tc_key.tc_idx >= r->num_implements) {
  //         return NULL;
  //       }
  //
  //       if (tc_key.meth_idx >= l->implements[tc_key.tc_idx]->num_methods ||
  //           tc_key.meth_idx >= r->implements[tc_key.tc_idx]->num_methods) {
  //         return NULL;
  //       }
  // #endif
  //
  //       TypeClass *tcl = l->implements[tc_key.tc_idx];
  //       TypeClass *tcr = r->implements[tc_key.tc_idx];
  //
  //       if (tcl->rank >= tcr->rank) {
  //         return tcl->methods + tc_key.meth_idx;
  //       }
  //       return tcr->methods + tc_key.meth_idx;
  //     }
  //   }
  // TODO: handle list prepend '::'

  return NULL;
}

LLVMTypeRef llvm_type_of_identifier(Ast *id, TypeEnv *env,
                                    LLVMModuleRef module) {
  if (id->tag == AST_VOID) {
    return LLVMVoidType();
  }

  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  Type *lookup_type = find_type_in_env(env, id->data.AST_IDENTIFIER.value);
  LLVMTypeRef t = type_to_llvm_type(lookup_type, env, module);
  return t;
}
