#include "backend_llvm/symbols.h"
#include "coroutines.h"
#include "function.h"
#include "globals.h"
#include "match.h"
#include "serde.h"
#include "types.h"
#include "types/type.h"
#include "variant.h"
#include <stdlib.h>
#include <string.h>

static bool is_coroutine_generator_symbol(Ast *id, JITLangCtx *ctx) {
  JITSymbol *sym = lookup_id_ast(id, ctx);
  return sym->type == STYPE_COROUTINE_GENERATOR;
}

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type) {
  JITSymbol *sym = malloc(sizeof(JITSymbol));
  sym->type = type_tag;
  sym->symbol_type = symbol_type;
  sym->val = val;
  sym->llvm_type = llvm_type;

  return sym;
}

JITSymbol *lookup_id_ast(Ast *ast, JITLangCtx *ctx) {

  if (ast->tag == AST_IDENTIFIER) {

    const char *chars = ast->data.AST_IDENTIFIER.value;
    int chars_len = ast->data.AST_IDENTIFIER.length;
    ObjString key = {.chars = chars, chars_len, hash_string(chars, chars_len)};
    int ptr = ctx->stack_ptr;

    while (ptr >= 0) {
      JITSymbol *sym = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
      if (sym != NULL) {
        return sym;
      }
      ptr--;
    }
  }

  return NULL;
}

JITSymbol *lookup_id_in_current_scope(Ast *ast, JITLangCtx *ctx) {
  if (ast->tag == AST_IDENTIFIER) {

    const char *chars = ast->data.AST_IDENTIFIER.value;
    int chars_len = ast->data.AST_IDENTIFIER.length;
    ObjString key = {.chars = chars, chars_len, hash_string(chars, chars_len)};
    int ptr = ctx->stack_ptr;

    JITSymbol *sym = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
    if (sym != NULL) {
      return sym;
    }
  }

  return NULL;
}

JITSymbol *sym_lookup_by_name_mut(ObjString key, JITLangCtx *ctx) {

  int ptr = ctx->stack_ptr;

  while (ptr >= 0) {
    JITSymbol *sym = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
    if (sym != NULL) {
      return sym;
    }
    ptr--;
  }
  return NULL;
}

int lookup_id_ast_in_place(Ast *ast, JITLangCtx *ctx, JITSymbol *sym) {

  if (ast->tag == AST_IDENTIFIER) {

    const char *chars = ast->data.AST_IDENTIFIER.value;
    int chars_len = ast->data.AST_IDENTIFIER.length;
    ObjString key = {.chars = chars, chars_len, hash_string(chars, chars_len)};
    int ptr = ctx->stack_ptr;

    while (ptr >= 0) {
      JITSymbol *_sym = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
      if (_sym != NULL) {
        *sym = *_sym;
        sym->type = _sym->type;

        return 0;
      }
      ptr--;
    }

    return 1;
  }
  return 1;
}

LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  const char *chars = ast->data.AST_IDENTIFIER.value;
  int length = ast->data.AST_IDENTIFIER.length;

  JITSymbol *sym = lookup_id_ast(ast, ctx);

  if (!sym) {
    LLVMValueRef enum_val =
        codegen_simple_enum_member(ast, ctx, module, builder);

    if (enum_val) {
      return enum_val;
    }
    fprintf(
        stderr,
        "codegen identifier failed symbol '%s' not found in scope %d %s:%d\n",
        chars, ctx->stack_ptr, __FILE__, __LINE__);
    return NULL;
  }
  switch (sym->type) {
  case STYPE_TOP_LEVEL_VAR: {
    return codegen_get_global(sym, module, builder);
  }
  case STYPE_FN_PARAM: {
    int idx = sym->symbol_data.STYPE_FN_PARAM;
    // return LLVMGetParam(current_func(builder), idx);
    return sym->val;
  }

  case STYPE_FUNCTION: {
    return sym->val;
  }

  case STYPE_GENERIC_FUNCTION: {
    return get_specific_callable(sym, chars, ast->md, ctx, module, builder);
  }

  case STYPE_GENERIC_COROUTINE_GENERATOR: {
    return get_specific_coroutine_generator_callable(sym, chars, ast->md, ctx,
                                                     module, builder);
  }

  case STYPE_LOCAL_VAR: {
    return sym->val;
  }

  default: {
    return sym->val;
  }
  }
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef create_generic_fn_binding(Ast *binding, Ast *fn_ast,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, fn_ast->md, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = fn_ast;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
              hash_string(id_chars, id_len), sym);

  return NULL;
}

LLVMValueRef create_generic_coroutine_binding(Ast *binding, Ast *fn_ast,
                                              JITLangCtx *ctx,
                                              LLVMModuleRef module,
                                              LLVMBuilderRef builder) {
  JITSymbol *sym =
      new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR, fn_ast->md, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.ast = fn_ast;
  sym->symbol_data.STYPE_GENERIC_COROUTINE_GENERATOR.stack_ptr = ctx->stack_ptr;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->stack + ctx->stack_ptr, id_chars,
              hash_string(id_chars, id_len), sym);

  return NULL;
}

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *outer_ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *binding = ast->data.AST_LET.binding;

  JITLangCtx cont_ctx = *outer_ctx;

  if (ast->data.AST_LET.in_expr != NULL) {
    cont_ctx = ctx_push(cont_ctx);
  }

  Type *expr_type = ast->data.AST_LET.expr->md;

  if (expr_type->kind == T_FN &&
      is_coroutine_generator(ast->data.AST_LET.expr->md) &&
      is_generic(ast->data.AST_LET.expr->md)) {
    return create_generic_coroutine_binding(binding, ast->data.AST_LET.expr,
                                            &cont_ctx, module, builder);
  }

  if (expr_type->kind == T_FN &&
      is_coroutine_generator(ast->data.AST_LET.expr->md)) {

    Type *def_type = ast->md;
    Type *instance_type = fn_return_type(def_type);
    LLVMTypeRef llvm_def_type;
    LLVMValueRef coroutine_func = coroutine_def(
        ast->data.AST_LET.expr, &cont_ctx, module, builder, &llvm_def_type);

    JITSymbol *def_sym = new_symbol(STYPE_COROUTINE_GENERATOR, def_type,
                                    coroutine_func, llvm_def_type);

    def_sym->symbol_data.STYPE_COROUTINE_GENERATOR.llvm_params_obj_type =
        type_to_llvm_type(instance_type->data.T_COROUTINE_INSTANCE.params_type,
                          cont_ctx.env, module);

    const char *id_chars = binding->data.AST_IDENTIFIER.value;
    int id_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(cont_ctx.stack + cont_ctx.stack_ptr, id_chars,
                hash_string(id_chars, id_len), def_sym);

    return coroutine_func;
  }

  if (expr_type->kind == T_FN && is_generic(expr_type)) {
    return create_generic_fn_binding(binding, ast->data.AST_LET.expr, &cont_ctx,
                                     module, builder);
  }

  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, outer_ctx, module, builder);

  if (!expr_val) {
    return NULL;
  }

  LLVMValueRef match_result =
      match_values(binding, expr_val, expr_type, &cont_ctx, module, builder);

  if (match_result == NULL) {
    fprintf(stderr, "Error: codegen for matching binding in let expression "
                    "failed\n");
    print_ast_err(ast);
    return NULL;
  }

  if (ast->data.AST_LET.in_expr != NULL) {
    LLVMValueRef res =
        codegen(ast->data.AST_LET.in_expr, &cont_ctx, module, builder);
    return res;
  }

  return expr_val;
}

Type *create_loop_sig_type() {
  Type *params_type = empty_type();
  params_type->kind = T_VAR;
  params_type->data.T_VAR = "params_obj";
  Type *ret = empty_type();
  ret->kind = T_VAR;
  ret->data.T_VAR = "cor_ret";
  Type *ret_opt = create_option_type(ret);
  Type *input_inst = empty_type();
  input_inst->kind = T_COROUTINE_INSTANCE;
  input_inst->data.T_COROUTINE_INSTANCE.params_type = params_type;
  input_inst->data.T_COROUTINE_INSTANCE.yield_interface =
      type_fn(&t_void, ret_opt);

  Type *input_def = type_fn(params_type, input_inst);
  Type *loop_sig = input_inst;
  loop_sig = type_fn(params_type, loop_sig);
  loop_sig = type_fn(input_def, loop_sig);

  return loop_sig;
}

Type *create_iter_map_sig_type() {
  // Type *input_param = tvar("cor_input_param");
  // Type *input_ret = tvar("cor_input_ret");
  //
  // Type *input_instance_type =
  //     create_coroutine_instance_type(input_param, input_ret);
  //
  // Type *output_ret = tvar("cor_output_ret");
  // Type *transform_func = type_fn(input_ret, output_ret);
  //
  // Type *output_instance_type =
  //     create_coroutine_instance_type(input_param, output_ret);
  // Type *map_sig = output_instance_type;
  // map_sig = type_fn(input_instance_type, map_sig);
  // map_sig = type_fn(transform_func, map_sig);
  //
  // return map_sig;
}

// Type *create_iter_zip_sig_type() {
//
//
//   Type *ret_type1 = tvar("cor_ret1");
//   Type *ret_opt1 = create_option_type(ret_type1);
//   Type *instance_type = type_fn(&t_void, create_option_type(ret_type1));
//
//   Type *ret_type2 = tvar("cor_ret2");
//   Type *ret_opt2 = create_option_type(ret_type2);
//   Type *instance_type2 = type_fn(&t_void, create_option_type(ret_type2));
//
//   Type *zip = create_tuple_type(2, )
// }

TypeEnv *initialize_builtin_funcs(ht *stack, TypeEnv *env) {
  for (int i = 0; i < _NUM_BINOPS; i++) {
    _binop_map bm = binop_map[i];
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, bm.binop_fn_type, NULL, NULL);
    ht_set_hash(stack, bm.name, hash_string(bm.name, strlen(bm.name)), sym);
  }

#define FN_SYMBOL(id, type)                                                    \
  env = env_extend(env, id, type);                                             \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_FUNCTION, type, NULL, NULL);             \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  FN_SYMBOL(TYPE_NAME_OP_AND, &t_bool_binop);
  FN_SYMBOL(TYPE_NAME_OP_OR, &t_bool_binop);

#define GENERIC_FN_SYMBOL(id, type)                                            \
  env = env_extend(env, id, type);                                             \
  ({                                                                           \
    JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, type, NULL, NULL);     \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_AT, &t_array_at_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_SIZE, &t_array_size_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_DATA_PTR, &t_array_data_ptr_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_INCR, &t_array_incr_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_SLICE, &t_array_slice_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_NEW, &t_array_new_fn_sig);

  GENERIC_FN_SYMBOL(SYM_NAME_ARRAY_TO_LIST, &t_array_to_list_fn_sig);

  // Commented section remains unchanged
  // Type *array_init_fn_sig =
  //     type_fn(&t_int, type_fn(&t_array_var_el, &t_array_var));
  // env = env_extend(env, "array_init", array_init_fn_sig);
  //
  // JITSymbol *array_init_sym =
  //     new_symbol(STYPE_GENERIC_FUNCTION, array_init_fn_sig, NULL, NULL);
  //
  // ht_set_hash(stack, "array_init", hash_string("array_init", 10),
  //             array_init_sym);

  JITSymbol *deref_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_ptr_deref_sig, NULL, NULL);

  ht_set_hash(stack, SYM_NAME_DEREF,
              hash_string(SYM_NAME_DEREF, strlen(SYM_NAME_DEREF)), deref_sym);

  env = env_extend(env, "string_add", &t_string_add_fn_sig);
  JITSymbol *string_add_sym =
      new_symbol(STYPE_FUNCTION, &t_string_add_fn_sig, NULL, NULL);
  ht_set_hash(stack, "string_add", hash_string("string_add", 10),
              string_add_sym);

#define GENERIC_COR_SYMBOL(id, type)                                           \
  env = env_extend(env, id, type);                                             \
  ({                                                                           \
    JITSymbol *sym =                                                           \
        new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR, type, NULL, NULL);       \
    ht_set_hash(stack, id, hash_string(id, strlen(id)), sym);                  \
  })

  // JITSymbol *iter_of_list_sym = new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR,
  //                                          &t_iter_of_list_sig, NULL, NULL);
  // ht_set_hash(stack, SYM_NAME_ITER_OF_LIST,
  //             hash_string(SYM_NAME_ITER_OF_LIST,
  //             strlen(SYM_NAME_ITER_OF_LIST)), iter_of_list_sym);

  // Commented section remains unchanged
  // env = env_extend(env, "iter_of_list_inf", &t_iter_of_list_sig);
  // JITSymbol *iter_of_list_inf_sym =
  // new_symbol(STYPE_GENERIC_COROUTINE_GENERATOR,
  //                                          &t_iter_of_list_sig, NULL, NULL);
  // ht_set_hash(stack, "iter_of_list_inf", hash_string("iter_of_list_inf", 16),
  //             iter_of_list_inf_sym);

  GENERIC_COR_SYMBOL(SYM_NAME_ITER_OF_ARRAY, &t_iter_of_array_sig);
  GENERIC_COR_SYMBOL(SYM_NAME_ITER_OF_LIST, &t_iter_of_list_sig);
  GENERIC_COR_SYMBOL(SYM_NAME_LOOP, &t_cor_loop_sig);
  GENERIC_COR_SYMBOL(SYM_NAME_MAP_ITER, &t_cor_map_iter_sig);

  // Type *t_iter_map_sig = create_iter_map_sig_type();
  // GENERIC_FN_SYMBOL(SYM_NAME_ITER_MAP, t_iter_map_sig);
  //
  return env;
}
