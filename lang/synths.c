#include "synths.h"
#include "backend_llvm/util.h"
#include "parse.h"
#include "types/type.h"
#include "types/util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

LLVMValueRef node_of_val_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("node_of_double", *fn_type, module);
}

LLVMValueRef const_node_of_val(LLVMValueRef val, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef node_of_double_func = node_of_val_fn(&fn_type, module);
  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  LLVMValueRef const_node = LLVMBuildCall2(
      builder, fn_type, node_of_double_func, &double_val, 1, "const_node");
  return const_node;
}

// Define the function pointer type
typedef LLVMValueRef (*SynthConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                        LLVMBuilderRef);

LLVMValueRef ConsSynth(LLVMValueRef value, Type *type_from,
                       LLVMModuleRef module, LLVMBuilderRef builder) {
  return const_node_of_val(value, module, builder);
}

Type t_synth = {T_CONS,
                {.T_CONS =
                     {
                         TYPE_NAME_PTR,
                         (Type *[]){&t_char},
                         1,
                     }},
                .alias = "Synth",
                .constructor = ConsSynth,
                .constructor_size = sizeof(SynthConsMethod)};

#define GENERATE_NODE_BINOP_FN_GETTER(name)                                    \
  LLVMValueRef get_##name##_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {   \
    LLVMValueRef node_func = LLVMGetNamedFunction(module, #name);              \
    LLVMTypeRef node_type = LLVMPointerType(LLVMInt8Type(), 0);                \
    *fn_type = LLVMFunctionType(node_type,                                     \
                                (LLVMTypeRef[]){node_type, node_type}, 2, 0);  \
    if (!node_func) {                                                          \
      node_func = LLVMAddFunction(module, #name, *fn_type);                    \
    }                                                                          \
    return node_func;                                                          \
  }

GENERATE_NODE_BINOP_FN_GETTER(sum2_node)
GENERATE_NODE_BINOP_FN_GETTER(sub2_node)
GENERATE_NODE_BINOP_FN_GETTER(mul2_node)
GENERATE_NODE_BINOP_FN_GETTER(div2_node)
GENERATE_NODE_BINOP_FN_GETTER(mod2_node)

static LLVMValueRef SYNTH_BINOP(LLVMValueRef fn, LLVMTypeRef fn_type,
                                LLVMValueRef lval, Type *ltype,
                                LLVMValueRef rval, Type *rtype,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  int type_check =
      (types_equal(ltype, &t_synth) << 1) | types_equal(rtype, &t_synth);

  switch (type_check) {
  case 0b11:
    return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                          "Synth_binop");
  case 0b10: {
    LLVMValueRef node_of_rval = const_node_of_val(rval, module, builder);
    printf("node of rval \n");

    return LLVMBuildCall2(builder, fn_type, fn,
                          (LLVMValueRef[]){lval, node_of_rval}, 2,
                          "Synth_binop");
  }
  case 0b01: {
    LLVMValueRef node_of_lval = const_node_of_val(lval, module, builder);
    return LLVMBuildCall2(builder, fn_type, fn,
                          (LLVMValueRef[]){node_of_lval, rval}, 2,
                          "Synth_binop");
  }
  default:
    fprintf(stderr, "Expected two Synth operands");
    return NULL;
  }
  return NULL;
}

static LLVMValueRef codegen_synth_plus(LLVMValueRef lval, Type *ltype,
                                       LLVMValueRef rval, Type *rtype,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  int type_check =
      (types_equal(ltype, &t_synth) << 1) | types_equal(rtype, &t_synth);

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_sum2_node_fn(&fn_type, module);
  return SYNTH_BINOP(fn, fn_type, lval, ltype, rval, rtype, module, builder);
}

static LLVMValueRef codegen_synth_minus(LLVMValueRef lval, Type *ltype,
                                        LLVMValueRef rval, Type *rtype,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_sub2_node_fn(&fn_type, module);
  return SYNTH_BINOP(fn, fn_type, lval, ltype, rval, rtype, module, builder);
}

static LLVMValueRef codegen_synth_mul(LLVMValueRef lval, Type *ltype,
                                      LLVMValueRef rval, Type *rtype,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_mul2_node_fn(&fn_type, module);
  return SYNTH_BINOP(fn, fn_type, lval, ltype, rval, rtype, module, builder);
}

static LLVMValueRef codegen_synth_div(LLVMValueRef lval, Type *ltype,
                                      LLVMValueRef rval, Type *rtype,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_div2_node_fn(&fn_type, module);
  return SYNTH_BINOP(fn, fn_type, lval, ltype, rval, rtype, module, builder);
}

static LLVMValueRef codegen_synth_mod(LLVMValueRef lval, Type *ltype,
                                      LLVMValueRef rval, Type *rtype,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_mod2_node_fn(&fn_type, module);
  return SYNTH_BINOP(fn, fn_type, lval, ltype, rval, rtype, module, builder);
}

// Define the function pointer type
typedef LLVMValueRef (*NumTypeClassMethod)(LLVMValueRef, Type *, LLVMValueRef,
                                           Type *, LLVMModuleRef,
                                           LLVMBuilderRef);

// clang-format off
static NumTypeClassMethod synth_num_methods[] = {
    [TOKEN_PLUS -   TOKEN_PLUS] = codegen_synth_plus,
    [TOKEN_MINUS -  TOKEN_PLUS] = codegen_synth_minus,
    [TOKEN_STAR -   TOKEN_PLUS] = codegen_synth_mul,
    [TOKEN_SLASH -  TOKEN_PLUS] = codegen_synth_div,
    [TOKEN_MODULO - TOKEN_PLUS] = codegen_synth_mod,
};

// clang-format on

TypeEnv *initialize_type_env_synth(TypeEnv *env) {
  TypeClass *synth_num_typeclass = typeclass_instance(&TCNum);

  synth_num_typeclass->methods = synth_num_methods;
  synth_num_typeclass->method_size = sizeof(NumTypeClassMethod);

  add_typeclass_impl(&t_synth, synth_num_typeclass);

  env = env_extend(env, "Synth", &t_synth);
  return env;
}
