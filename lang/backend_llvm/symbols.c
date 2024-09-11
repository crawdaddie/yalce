#include "backend_llvm/symbols.h"
#include "globals.h"
#include "match.h"
#include "serde.h"
#include "variant.h"
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

  JITSymbol *res = NULL;

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
    return NULL;
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
