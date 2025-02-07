#include "synths.h";
#include "serde.h"
#include "application.h"
#include "codegen.h"
#include "ht.h"
#include "symbols.h"
#include "types/inference.h"
#include "util.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <string.h>

LLVMValueRef ConsSynth(LLVMValueRef input, Type *input_type,
                       LLVMModuleRef module, LLVMBuilderRef builder);

bool is_synth_type(Type *t) {
  return t->alias && (strcmp(t->alias, "Synth") == 0);
}

bool is_signal_type(Type *t) {
  return t->alias && (strcmp(t->alias, "Signal") == 0);
}

Type t_synth = {T_CONS,
                {.T_CONS =
                     {
                         TYPE_NAME_PTR,
                         (Type *[]){&t_char},
                         1,
                     }},
                .alias = "Synth",

                .constructor = ConsSynth};

LLVMValueRef const_sig_of_val(LLVMValueRef val, LLVMModuleRef module,
                              LLVMBuilderRef builder) {
  LLVMTypeRef fn_type =
      LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                       (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  LLVMValueRef sig_of_val_func =
      get_extern_fn("signal_of_double", fn_type, module);

  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");

  LLVMValueRef const_sig = LLVMBuildCall2(builder, fn_type, sig_of_val_func,
                                          &double_val, 1, "sig_of_val");
  return const_sig;
}

LLVMValueRef const_node_of_val(LLVMValueRef val, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  LLVMTypeRef fn_type;

  fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                             (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

  LLVMValueRef node_of_double_func =
      get_extern_fn("node_of_double", fn_type, module);

  LLVMValueRef double_val =
      LLVMBuildSIToFP(builder, val, LLVMDoubleType(), "cast_to_double");
  LLVMValueRef const_node = LLVMBuildCall2(
      builder, fn_type, node_of_double_func, &double_val, 1, "const_node");
  return const_node;
}

LLVMValueRef ConsSynth(LLVMValueRef input, Type *input_type,
                       LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (input_type->kind) {
  case T_INT:
  case T_NUM: {
    return const_node_of_val(input, module, builder);
  }
  case T_CONS: {
    if (is_synth_type(input_type)) {
      return input;
    }
    if (is_signal_type(input_type)) {

      LLVMTypeRef fn_type =
          LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                           (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

      LLVMValueRef f = get_extern_fn("node_of_sig", fn_type, module);

      return LLVMBuildCall2(builder, fn_type, f, (LLVMValueRef[]){input}, 1,
                            "get_node_of_sig");
    }
  }
  default: {
    return NULL;
  }
  }
}

LLVMValueRef ConsSignal(LLVMValueRef input, Type *input_type,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (input_type->kind) {
  case T_INT:
  case T_NUM: {
    return const_sig_of_val(input, module, builder);
  }
  case T_CONS: {
    if (is_synth_type(input_type)) {
      LLVMTypeRef fn_type =
          LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0),
                           (LLVMTypeRef[]){LLVMDoubleType()}, 1, 0);

      LLVMValueRef f = get_extern_fn("out_sig", fn_type, module);

      return LLVMBuildCall2(builder, fn_type, f, (LLVMValueRef[]){input}, 1,
                            "get_out_signal_of_node");
    }

    if (is_signal_type(input_type)) {
      return input;
    }
  }
  default: {
    return NULL;
  }
  }
}

Type t_signal = {T_CONS,
                 {.T_CONS =
                      {
                          TYPE_NAME_PTR,
                          (Type *[]){&t_char},
                          1,
                      }},
                 .alias = "Signal",
                 .constructor = ConsSignal};

Type t_synth_arithmetic_sig = {
    T_FN,
    {.T_FN = {
         .from = &t_synth,
         .to = &(Type){T_FN, {.T_FN = {.from = &t_synth, .to = &t_synth}}}}}};

Type t_signal_arithmetic_sig = {
    T_FN,
    {.T_FN = {
         .from = &t_signal,
         .to = &(Type){T_FN, {.T_FN = {.from = &t_signal, .to = &t_signal}}}}}};

#define SYNTH_ARITHMETIC(_native_fn_name, _ast, _ctx, _module, _builder)       \
  ({                                                                           \
    Type *ltype = _ast->data.AST_APPLICATION.args->md;                         \
    Type *rtype = (_ast->data.AST_APPLICATION.args + 1)->md;                   \
    LLVMValueRef l =                                                           \
        codegen(_ast->data.AST_APPLICATION.args, _ctx, _module, builder);      \
    l = handle_type_conversions(l, ltype, &t_synth, _module, _builder);        \
    LLVMValueRef r =                                                           \
        codegen(ast->data.AST_APPLICATION.args + 1, _ctx, _module, _builder);  \
    r = handle_type_conversions(r, rtype, &t_synth, _module, _builder);        \
    LLVMTypeRef fn_type = LLVMFunctionType(                                    \
        GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);         \
    LLVMValueRef fn = get_extern_fn(_native_fn_name, fn_type, _module);        \
    LLVMBuildCall2(_builder, fn_type, fn, (LLVMValueRef[]){l, r}, 2,           \
                   _native_fn_name);                                           \
  })

LLVMValueRef SynthSumHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef call = SYNTH_ARITHMETIC("sum2_node", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SynthSubHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef call = SYNTH_ARITHMETIC("sub2_node", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SynthMulHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef call = SYNTH_ARITHMETIC("mul2_node", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SynthDivHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef call = SYNTH_ARITHMETIC("div2_node", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SynthModHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef call = SYNTH_ARITHMETIC("mod2_node", ast, ctx, module, builder);
  return call;
}

#define SIGNAL_ARITHMETIC(_native_fn_name, _ast, _ctx, _module, _builder)       \
  ({                                                                           \
    Type *ltype = _ast->data.AST_APPLICATION.args->md;                         \
    Type *rtype = (_ast->data.AST_APPLICATION.args + 1)->md;                   \
    LLVMValueRef l =                                                           \
        codegen(_ast->data.AST_APPLICATION.args, _ctx, _module, builder);      \
    l = handle_type_conversions(l, ltype, &t_synth, _module, _builder);        \
    LLVMValueRef r =                                                           \
        codegen(ast->data.AST_APPLICATION.args + 1, _ctx, _module, _builder);  \
    r = handle_type_conversions(r, rtype, &t_synth, _module, _builder);        \
    LLVMTypeRef fn_type = LLVMFunctionType(                                    \
        GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR}, 2, 0);         \
    LLVMValueRef fn = get_extern_fn(_native_fn_name, fn_type, _module);        \
    LLVMBuildCall2(_builder, fn_type, fn, (LLVMValueRef[]){l, r}, 2,           \
                   _native_fn_name);                                           \
  })

LLVMValueRef SignalSumHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef call = SIGNAL_ARITHMETIC("sum2_sigs", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SignalSubHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef call = SIGNAL_ARITHMETIC("sub2_sigs", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SignalMulHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  // printf("signal mul handler\n");
  // print_ast(ast);
  LLVMValueRef call = SIGNAL_ARITHMETIC("mul2_sigs", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SignalDivHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {

  LLVMValueRef call = SIGNAL_ARITHMETIC("div2_sigs", ast, ctx, module, builder);
  return call;
}

LLVMValueRef SignalModHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                             LLVMBuilderRef builder) {
  LLVMValueRef call = SIGNAL_ARITHMETIC("mod2_sigs", ast, ctx, module, builder);
  return call;
}

void initialize_synth_types(JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {

  ht *stack = (ctx->frame->table);
#define GENERIC_FN_SYMBOL(id, type, _builtin_handler)                          \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, type, NULL, NULL);     \
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =                  \
        _builtin_handler;                                                      \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  add_builtin("Synth", &t_synth);
  static TypeClass tc_synth[] = {{
                                     .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                     .rank = 5.0,
                                 },
                                 {
                                     .name = TYPE_NAME_TYPECLASS_ORD,
                                     .rank = 5.0,
                                 },
                                 {
                                     .name = TYPE_NAME_TYPECLASS_EQ,
                                     .rank = 5.0,
                                 }};
  typeclasses_extend(&t_synth, tc_synth);
  typeclasses_extend(&t_synth, tc_synth + 1);
  typeclasses_extend(&t_synth, tc_synth + 2);
  GENERIC_FN_SYMBOL("Synth.+", &t_synth_arithmetic_sig, SynthSumHandler);
  GENERIC_FN_SYMBOL("Synth.-", &t_synth_arithmetic_sig, SynthSubHandler);
  GENERIC_FN_SYMBOL("Synth.*", &t_synth_arithmetic_sig, SynthMulHandler);
  GENERIC_FN_SYMBOL("Synth./", &t_synth_arithmetic_sig, SynthDivHandler);
  GENERIC_FN_SYMBOL("Synth.%", &t_synth_arithmetic_sig, SynthModHandler);

  add_builtin("Signal", &t_signal);
  static TypeClass tc_signal[] = {{
                                      .name = TYPE_NAME_TYPECLASS_ARITHMETIC,
                                      .rank = 4.9,
                                  },
                                  {
                                      .name = TYPE_NAME_TYPECLASS_ORD,
                                      .rank = 4.9,
                                  },
                                  {
                                      .name = TYPE_NAME_TYPECLASS_EQ,
                                      .rank = 4.9,
                                  }};
  typeclasses_extend(&t_signal, tc_signal);
  typeclasses_extend(&t_signal, tc_signal + 1);
  typeclasses_extend(&t_signal, tc_signal + 2);
  GENERIC_FN_SYMBOL("Signal.+", &t_signal_arithmetic_sig, SignalSumHandler);
  GENERIC_FN_SYMBOL("Signal.-", &t_signal_arithmetic_sig, SignalSubHandler);
  GENERIC_FN_SYMBOL("Signal.*", &t_signal_arithmetic_sig, SignalMulHandler);
  GENERIC_FN_SYMBOL("Signal./", &t_signal_arithmetic_sig, SignalDivHandler);
  GENERIC_FN_SYMBOL("Signal.%", &t_signal_arithmetic_sig, SignalModHandler);
}
