#include "backend_llvm/symbols.h"
#include "function.h"
#include "globals.h"
#include "match.h"
#include "serde.h"
#include "types/type.h"
#include "variant.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

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
    // printf("lookup sym: ");
    // print_ast(ast);

    const char *chars = ast->data.AST_IDENTIFIER.value;
    int chars_len = ast->data.AST_IDENTIFIER.length;
    ObjString key = {.chars = chars, chars_len, hash_string(chars, chars_len)};
    int ptr = ctx->stack_ptr;

    while (ptr >= 0) {
      JITSymbol *_sym = ht_get_hash(ctx->stack + ptr, key.chars, key.hash);
      // printf("find sym in stack %d\n", ptr);
      if (_sym != NULL) {
        *sym = *_sym;
        sym->type = _sym->type;

        // printf("sym %s -> %d stack %d hash %llu\n", key.chars, sym->type,
        // ptr,
        //        key.hash);
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

  case STYPE_LOCAL_VAR: {
    return sym->val;
  }

  default: {
    return NULL;
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

LLVMValueRef codegen_assignment(Ast *ast, JITLangCtx *outer_ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *binding = ast->data.AST_LET.binding;

  JITLangCtx cont_ctx = *outer_ctx;

  if (ast->data.AST_LET.in_expr != NULL) {
    cont_ctx = ctx_push(cont_ctx);
  }

  Type *expr_type = ast->data.AST_LET.expr->md;

  if (expr_type->kind == T_FN && is_generic(expr_type)) {

    return create_generic_fn_binding(binding, ast->data.AST_LET.expr, &cont_ctx,
                                     module, builder);
  }

  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, outer_ctx, module, builder);

  // LLVMDumpValue(expr_val);
  if (!expr_val) {
    return NULL;
  }

  // if (binding->tag == AST_IDENTIFIER &&
  //     strcmp("x", binding->data.AST_IDENTIFIER.value) == 0) {
  //   printf("set up binding for val x (scope %d) %d\n", outer_ctx->stack_ptr,
  //          binding->tag);
  //   print_ast(binding);
  // }

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

TypeEnv *initialize_builtin_funcs(ht *stack, TypeEnv *env) {
  for (int i = 0; i < _NUM_BINOPS; i++) {
    _binop_map bm = binop_map[i];
    JITSymbol *sym =
        new_symbol(STYPE_GENERIC_FUNCTION, bm.binop_fn_type, NULL, NULL);
    ht_set_hash(stack, bm.name, hash_string(bm.name, strlen(bm.name)), sym);
  }

  env = env_extend(env, "array_at", &t_array_at_fn_sig);
  JITSymbol *array_at_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_at_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_at", hash_string("array_at", 8), array_at_sym);

  env = env_extend(env, "array_size", &t_array_size_fn_sig);
  JITSymbol *array_size_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_size_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_size", hash_string("array_size", 10),
              array_size_sym);

  env = env_extend(env, "array_data_ptr", &t_array_data_ptr_fn_sig);
  JITSymbol *array_data_ptr_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_data_ptr_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_data_ptr", hash_string("array_data_ptr", 14),
              array_data_ptr_sym);

  env = env_extend(env, "array_incr", &t_array_incr_fn_sig);
  JITSymbol *array_incr_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_incr_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_incr", hash_string("array_incr", 10),
              array_incr_sym);

  env = env_extend(env, "array_slice", &t_array_slice_fn_sig);
  JITSymbol *array_slice_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_slice_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_slice", hash_string("array_slice", 11),
              array_slice_sym);

  env = env_extend(env, "array_new", &t_array_new_fn_sig);
  JITSymbol *array_new_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_new_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_new", hash_string("array_new", 9), array_new_sym);

  env = env_extend(env, "array_to_list", &t_array_to_list_fn_sig);
  JITSymbol *array_to_list_sym =
      new_symbol(STYPE_GENERIC_FUNCTION, &t_array_to_list_fn_sig, NULL, NULL);
  ht_set_hash(stack, "array_to_list", hash_string("array_to_list", 10),
              array_to_list_sym);

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

  ht_set_hash(stack, "deref", hash_string("deref", 5), deref_sym);

  env = env_extend(env, "string_add", &t_string_add_fn_sig);
  JITSymbol *string_add_sym =
      new_symbol(STYPE_FUNCTION, &t_string_add_fn_sig, NULL, NULL);
  ht_set_hash(stack, "string_add", hash_string("string_add", 10),
              string_add_sym);

  return env;
}
