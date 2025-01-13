#include "backend_llvm/symbols.h"
#include "backend_llvm/builtin_functions.h"
#include "function.h"
#include "globals.h"
#include "match.h"
#include "serde.h"
#include "types.h"
#include "types/type.h"
#include "variant.h"
#include "llvm-c/Core.h"
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
    Type *enum_type = env_lookup(ctx->env, chars);
    if (!enum_type) {
      enum_type = get_builtin_type(chars);
    }

    if (!enum_type) {
      fprintf(
          stderr,
          "codegen identifier failed symbol '%s' not found in scope %d %s:%d\n",
          chars, ctx->stack_ptr, __FILE__, __LINE__);
      return NULL;
    }

    if (is_simple_enum(enum_type)) {
      int vidx;
      for (vidx = 0; vidx < enum_type->data.T_CONS.num_args; vidx++) {
        if (strcmp(chars,
                   enum_type->data.T_CONS.args[vidx]->data.T_CONS.name) == 0) {
          break;
        }
      }
      return LLVMConstInt(LLVMInt8Type(), vidx, 0);
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

    // case STYPE_GENERIC_COROUTINE_GENERATOR: {
    //   return get_specific_coroutine_generator_callable(sym, chars, ast->md,
    //   ctx,
    //                                                    module, builder);
    // }

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

LLVMValueRef create_fn_binding(Ast *binding, Type *fn_type, LLVMValueRef fn,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {
  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, fn, NULL);

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
  if (expr_type->kind == T_FN && is_generic(expr_type)) {
    return create_generic_fn_binding(binding, ast->data.AST_LET.expr, &cont_ctx,
                                     module, builder);
  }

  if (expr_type->kind == T_FN) {
    return create_fn_binding(
        binding, expr_type,
        codegen_fn(ast->data.AST_LET.expr, outer_ctx, module, builder),
        &cont_ctx, module, builder);
  }

  LLVMValueRef expr_val =
      codegen(ast->data.AST_LET.expr, outer_ctx, module, builder);

  if (!expr_val) {
    return NULL;
  }

  LLVMValueRef match_result = codegen_pattern_binding(
      binding, expr_val, expr_type, &cont_ctx, module, builder);

  if (match_result == NULL) {
    fprintf(stderr, "Error: codegen for pattern binding in let expression "
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
