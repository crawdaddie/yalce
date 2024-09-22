#include "synths.h"
#include "backend_llvm/util.h"
#include "parse.h"
#include "types/type.h"
#include "types/typeclass.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef node_of_val_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("node_of_double", *fn_type, module);
}

LLVMValueRef node_of_sig_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("node_of_sig", *fn_type, module);
}

LLVMValueRef sig_of_val_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);
  return get_extern_fn("signal_of_double", *fn_type, module);
}

LLVMValueRef out_sig_of_node_fn(LLVMTypeRef *fn_type, LLVMModuleRef module) {

  *fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                              (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  return get_extern_fn("out_sig", *fn_type, module);
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

LLVMValueRef const_node_of_sig(LLVMValueRef val, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef node_of_sig_func = node_of_sig_fn(&fn_type, module);
  LLVMValueRef const_node =
      LLVMBuildCall2(builder, fn_type, node_of_sig_func, &val, 1, "const_node");
  return const_node;
}

LLVMValueRef const_sig_of_val_int(LLVMValueRef val, LLVMModuleRef module,
                                  LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef sig_of_val_func = sig_of_val_fn(&fn_type, module);
  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  LLVMValueRef const_sig = LLVMBuildCall2(builder, fn_type, sig_of_val_func,
                                          &double_val, 1, "sig_of_val");
  return const_sig;
}

LLVMValueRef const_sig_of_val(LLVMValueRef val, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef sig_of_val_func = sig_of_val_fn(&fn_type, module);

  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");

  LLVMValueRef const_sig = LLVMBuildCall2(builder, fn_type, sig_of_val_func,
                                          &double_val, 1, "sig_of_val");
  return const_sig;
}

LLVMValueRef out_sig_of_node_val(LLVMValueRef val, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;
  LLVMValueRef out_sig_of_node_func = out_sig_of_node_fn(&fn_type, module);
  LLVMValueRef const_sig = LLVMBuildCall2(
      builder, fn_type, out_sig_of_node_func, &val, 1, "sig_of_val");
  return const_sig;
}

// Define the function pointer type
typedef LLVMValueRef (*SynthConsMethod)(LLVMValueRef, Type *, LLVMModuleRef,
                                        LLVMBuilderRef);

LLVMValueRef ConsSynth(LLVMValueRef value, Type *type_from,
                       LLVMModuleRef module, LLVMBuilderRef builder) {

  if (type_from->alias && strcmp(type_from->alias, "Signal") == 0) {
    return const_node_of_sig(value, module, builder);
  }

  return const_node_of_val(value, module, builder);
}

LLVMValueRef ConsSignal(LLVMValueRef value, Type *type_from,
                        LLVMModuleRef module, LLVMBuilderRef builder) {

  print_type(type_from);

  switch (type_from->kind) {
  case T_INT:
  case T_NUM: {
    return const_sig_of_val(value, module, builder);
  }
  case T_CONS: {
    if (type_from->alias && (strcmp(type_from->alias, "Synth") == 0)) {
      return out_sig_of_node_val(value, module, builder);
    }

    if (type_from->alias && (strcmp(type_from->alias, "Signal") == 0)) {
      return value;
    }
  }
  default: {
    return NULL;
  }
  }
}

TypeClass TCEq_synth = {
    "eq", .num_methods = 2, .rank = 2.0,
    .methods = (Method[]){
        (Method){"==",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},
        (Method){"!=",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},
    }};

TypeClass TCOrd_synth = {
    "ord", .num_methods = 4, .rank = 2.0,
    .methods = (Method[]){
        (Method){">",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"<",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){">=",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"<=",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},
    }};

TypeClass TCArithmetic_synth = {
    "arithmetic", .num_methods = 5, .rank = 2.0,
    .methods = (Method[]){
        (Method){"+",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"-",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"*",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"/",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},

        (Method){"%",
                 .signature = &MAKE_FN_TYPE_3(&t_synth, &t_synth, &t_synth)},
    }};

Type t_synth = {T_CONS,
                {.T_CONS =
                     {
                         TYPE_NAME_PTR,
                         (Type *[]){&t_char},
                         1,
                     }},
                .alias = "Synth",
                .constructor = ConsSynth,
                .constructor_size = sizeof(SynthConsMethod),
                .num_implements = 3,
                .implements = (TypeClass *[]){
                    &TCEq_synth,
                    &TCOrd_synth,
                    &TCArithmetic_synth,
                }};

Type t_signal = {T_CONS,
                 {.T_CONS =
                      {
                          TYPE_NAME_PTR,
                          (Type *[]){&t_char},
                          1,
                      }},
                 .alias = "Signal",
                 .constructor = ConsSignal,
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
  default: {
    fprintf(stderr, "Expected two Synth operands");
    return NULL;
  }
  }
  return NULL;
}

static LLVMValueRef codegen_synth_plus(LLVMValueRef lval, LLVMValueRef rval,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_sum2_node_fn(&fn_type, module);
  return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                        "Synth_binop");
}

static LLVMValueRef codegen_synth_minus(LLVMValueRef lval, LLVMValueRef rval,
                                        LLVMModuleRef module,
                                        LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_sub2_node_fn(&fn_type, module);

  return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                        "Synth_binop");
}

static LLVMValueRef codegen_synth_mul(LLVMValueRef lval, LLVMValueRef rval,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_mul2_node_fn(&fn_type, module);

  return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                        "Synth_binop");
}

static LLVMValueRef codegen_synth_div(LLVMValueRef lval, LLVMValueRef rval,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_div2_node_fn(&fn_type, module);

  return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                        "Synth_binop");
}

static LLVMValueRef codegen_synth_mod(LLVMValueRef lval, LLVMValueRef rval,
                                      LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  LLVMTypeRef fn_type;
  LLVMValueRef fn = get_mod2_node_fn(&fn_type, module);

  return LLVMBuildCall2(builder, fn_type, fn, (LLVMValueRef[]){lval, rval}, 2,
                        "Synth_binop");
}

// Define the function pointer type
typedef LLVMValueRef (*NumTypeClassMethod)(LLVMValueRef, Type *, LLVMValueRef,
                                           Type *, LLVMModuleRef,
                                           LLVMBuilderRef);

// // clang-format off
// static NumTypeClassMethod synth_num_methods[] = {
//     [TOKEN_PLUS -   TOKEN_PLUS] = codegen_synth_plus,
//     [TOKEN_MINUS -  TOKEN_PLUS] = codegen_synth_minus,
//     [TOKEN_STAR -   TOKEN_PLUS] = codegen_synth_mul,
//     [TOKEN_SLASH -  TOKEN_PLUS] = codegen_synth_div,
//     [TOKEN_MODULO - TOKEN_PLUS] = codegen_synth_mod,
// };

// clang-format on

TypeEnv *initialize_type_env_synth(TypeEnv *env) {
  // TypeClass *synth_num_typeclass = typeclass_instance(&TCNum);
  //
  // synth_num_typeclass->methods = synth_num_methods;
  // synth_num_typeclass->method_size = sizeof(NumTypeClassMethod);
  //
  // add_typeclass_impl(&t_synth, synth_num_typeclass);
  t_synth.implements[2]->methods[0].method = codegen_synth_plus;
  t_synth.implements[2]->methods[1].method = codegen_synth_minus;
  t_synth.implements[2]->methods[2].method = codegen_synth_mul;
  t_synth.implements[2]->methods[3].method = codegen_synth_div;
  t_synth.implements[2]->methods[4].method = codegen_synth_mod;
  env = env_extend(env, "Synth", &t_synth);

  env = env_extend(env, "Signal", &t_signal);

  return env;
}
