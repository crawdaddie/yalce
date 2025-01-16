#include "backend_llvm/symbols.h"

#include "adt.h"
#include "function.h"
#include "globals.h"
#include "ht.h"
#include "match.h"
#include "serde.h"
#include "types/inference.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);
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

    return find_in_ctx(chars, chars_len, ctx);
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

LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  const char *chars = ast->data.AST_IDENTIFIER.value;
  int length = ast->data.AST_IDENTIFIER.length;

  JITSymbol *sym = lookup_id_ast(ast, ctx);

  if (!sym) {
    Type *enum_type = env_lookup(ctx->env, chars);

    if (!enum_type) {
      fprintf(
          stderr,
          "codegen identifier failed enum '%s' not found in scope %d %s:%d\n",
          chars, ctx->stack_ptr, __FILE__, __LINE__);
      return NULL;
    }

    if (is_simple_enum(enum_type)) {
      return codegen_simple_enum_member(enum_type, chars, ctx, module, builder);
    } else {
      return codegen_adt_member(enum_type, chars, ctx, module, builder);
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

  return fn;
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

LLVMValueRef _codegen_let_expr(Ast *binding, Ast *expr, Ast *in_expr,
                               JITLangCtx *outer_ctx, JITLangCtx *inner_ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  Type *expr_type = expr->md;
  if (expr_type->kind == T_FN && is_generic(expr_type)) {
    return create_generic_fn_binding(binding, expr, inner_ctx, module, builder);
  }

  if (expr_type->kind == T_FN) {

    LLVMValueRef res = create_fn_binding(
        binding, expr_type, codegen_fn(expr, outer_ctx, module, builder),
        inner_ctx, module, builder);
    return res;
  }

  LLVMValueRef expr_val = codegen(expr, outer_ctx, module, builder);

  if (!expr_val) {
    return NULL;
  }

  LLVMValueRef match_result = codegen_pattern_binding(
      binding, expr_val, expr_type, inner_ctx, module, builder);

  if (match_result == NULL) {
    fprintf(stderr, "Error: codegen for pattern binding in let expression "
                    "failed\n");
    print_ast_err(binding);
    print_ast_err(expr);
    return NULL;
  }

  if (in_expr != NULL) {
    LLVMValueRef res = codegen(in_expr, inner_ctx, module, builder);
    return res;
  }

  return expr_val;
}

LLVMValueRef codegen_let_expr(Ast *ast, JITLangCtx *outer_ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *binding = ast->data.AST_LET.binding;

  JITLangCtx cont_ctx = *outer_ctx;

  if (ast->data.AST_LET.in_expr != NULL) {
    cont_ctx = ctx_push(cont_ctx);
  }
  return _codegen_let_expr(binding, ast->data.AST_LET.expr,
                           ast->data.AST_LET.in_expr, outer_ctx, &cont_ctx,
                           module, builder);
}
