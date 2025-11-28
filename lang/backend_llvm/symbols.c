#include "backend_llvm/symbols.h"

#include "adt.h"
#include "application.h"
#include "binding.h"
#include "closures.h"
#include "codegen.h"
#include "coroutines.h"
#include "function.h"
#include "function_extern.h"
#include "globals.h"
#include "ht.h"
#include "module.h"
#include "serde.h"
#include "types.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type.h"
#include "types/type_ser.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type) {

  JITSymbol *sym = malloc(sizeof(JITSymbol));
  sym->type = type_tag;
  sym->symbol_type = symbol_type;
  sym->val = val;
  sym->llvm_type = llvm_type;
  // TODO: if it's a symbol do I need to create a storage class???

  return sym;
}

JITSymbol *lookup_id_ast(Ast *ast, JITLangCtx *ctx) {

  if (ast->tag == AST_IDENTIFIER) {

    const char *chars = ast->data.AST_IDENTIFIER.value;
    int chars_len = ast->data.AST_IDENTIFIER.length;

    return find_in_ctx(chars, chars_len, ctx);
  }

  if (ast->tag == AST_RECORD_ACCESS) {

    JITSymbol *record_symbol =
        lookup_id_ast(ast->data.AST_RECORD_ACCESS.record, ctx);

    if (!record_symbol) {
      fprintf(stderr, "Error: record %s not found in scope %d",
              ast->data.AST_RECORD_ACCESS.record->data.AST_IDENTIFIER.value,
              ctx->stack_ptr);
      print_location(__current_ast);
      return NULL;
    }

    if (record_symbol->type == STYPE_MODULE) {
      JITSymbol *member_symbol =
          lookup_id_ast(ast->data.AST_RECORD_ACCESS.member,
                        record_symbol->symbol_data.STYPE_MODULE.ctx);
      return member_symbol;
    }
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
      enum_type = lookup_builtin_type(chars);
    }
    if (!enum_type) {
      fprintf(
          stderr,
          "codegen identifier failed enum '%s' not found in scope %d %s:%d\n",
          chars, ctx->stack_ptr, __FILE__, __LINE__);
      print_codegen_location();
      return NULL;
    }

    if (is_simple_enum(enum_type)) {
      return codegen_simple_enum_member(enum_type, chars, ctx, module, builder);

    } else if (strcmp(chars, "None") == 0) {

      LLVMTypeRef llvm_type = type_to_llvm_type(ast->type, ctx, module);

      if (!llvm_type) {
        print_location(ast);
        fprintf(stderr, "Option type not found\n");
        return NULL;
      }

      LLVMValueRef v = LLVMGetUndef(llvm_type);
      v = LLVMBuildInsertValue(builder, v, LLVMConstInt(LLVMInt8Type(), 1, 0),
                               0, "insert None tag");
      return v;

    } else {
      return codegen_adt_member(enum_type, chars, ctx, module, builder);
    }

    fprintf(
        stderr,
        "codegen identifier failed symbol '%s' not found in scope %d %s:%d\n",
        chars, ctx->stack_ptr, __FILE__, __LINE__);

    print_codegen_location();
    return NULL;
  }

  switch (sym->type) {
  case STYPE_TOP_LEVEL_VAR: {
    return codegen_get_global(chars, sym, module, builder);
  }

  case STYPE_FUNCTION: {
    return sym->val;
  }

  case STYPE_LAZY_EXTERN_FUNCTION: {
    if (sym->val) {
      return sym->val;
    }

    LLVMValueRef val = codegen_extern_fn(
        sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast, ctx, module, builder);
    sym->val = val;

    return sym->val;
  }

  case STYPE_GENERIC_FUNCTION: {
    LLVMValueRef f =
        get_specific_callable(sym, ast->type, ctx, module, builder);
    return f;
  }

  case STYPE_LOCAL_VAR: {

    if (sym->storage != NULL) {
      LLVMTypeRef llvm_type = type_to_llvm_type(ast->type, ctx, module);
      return LLVMBuildLoad2(builder, llvm_type, sym->storage, "load pointer");
    }

    return sym->val;
  }

  case STYPE_VARIANT_TYPE: {
    return codegen_adt_member(ast->type, chars, ctx, module, builder);
  }

  default: {
    return sym->val;
  }
  }
}

JITSymbol *create_generic_fn_symbol(Ast *fn_ast, JITLangCtx *ctx) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, fn_ast->type, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = fn_ast;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame = ctx->frame;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env = ctx->env;
  return sym;
}

LLVMValueRef create_generic_fn_binding(Ast *binding, Ast *fn_ast,
                                       JITLangCtx *ctx) {
  JITSymbol *sym = create_generic_fn_symbol(fn_ast, ctx);

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);

  return NULL;
}

LLVMValueRef create_fn_binding(Ast *binding, Type *fn_type, LLVMValueRef fn,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, fn, NULL);

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);

  return fn;
}

LLVMValueRef create_lazy_extern_fn_binding(Ast *binding, Ast *expr,
                                           Type *fn_type, LLVMValueRef fn,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  JITSymbol *sym = malloc(sizeof(JITSymbol));
  sym->type = STYPE_LAZY_EXTERN_FUNCTION;
  sym->symbol_type = fn_type;
  sym->val = NULL;
  sym->llvm_type = NULL;
  sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast = expr;

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);

  return fn;
}

bool is_array_at(Ast *expr) {
  if (expr->tag == AST_APPLICATION) {
    if (expr->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
      if (strcmp("array_at", expr->data.AST_APPLICATION.function->data
                                 .AST_IDENTIFIER.value) == 0) {
        return true;
      }
    }
  }
  return false;
}
Type *array_type(Ast *expr) {
  Type *arr = expr->data.AST_APPLICATION.args->type;
  if (arr->kind == T_CONS && is_array_type(arr) &&
      arr->data.T_CONS.args[0]->kind == T_FN) {
    return arr->data.T_CONS.args[0];
  }
  return NULL;
}

LLVMValueRef _codegen_let_expr(Ast *binding, Ast *expr, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {

  LLVMValueRef expr_val;
  Type *expr_type = expr->type;

  if (expr->tag == AST_APPLICATION && is_array_at(expr)) {
    Type *fn_type = array_type(expr);
    if (fn_type && !is_generic(fn_type)) {

      expr_val = create_fn_binding(binding, expr_type,
                                   codegen(expr, ctx, module, builder), ctx,
                                   module, builder);

      return expr_val;
    }

    if (fn_type && is_generic(fn_type)) {

      expr_val = create_generic_fn_binding(binding, expr, ctx);

      return expr_val;
    }
  }

  if (is_closure(expr_type) && application_is_partial(expr)) {
    LLVMValueRef closure_obj =
        create_closure_symbol(binding, expr, ctx, module, builder);

    return closure_obj;
  }

  if (binding->tag == AST_IDENTIFIER && expr->tag == AST_IDENTIFIER) {
    JITSymbol *sym = lookup_id_ast(expr, ctx);
    if (sym) {
      // create symbol alias - ie rebind symbol to another name
      // printf("create symbol alias - ie rebind symbol to another name\n");
      const char *chars = binding->data.AST_IDENTIFIER.value;
      int len = binding->data.AST_IDENTIFIER.length;
      ht_set_hash(ctx->frame->table, chars, hash_string(chars, len), sym);

      return sym->val;
    }
  }

  if (is_coroutine_constructor_type(expr_type)) {
    // if (expr_type->kind == T_FN &&
    // is_coroutine_constructor_type(expr_type))
    // {
    if (is_generic(expr_type)) {
      expr_val = create_generic_fn_binding(binding, expr, ctx);
    } else {
      expr_val = compile_coroutine(expr, ctx, module, builder);
      expr_val =
          create_fn_binding(binding, expr_type, expr_val, ctx, module, builder);
    }

    return expr_val;
  }

  if (expr_type->kind == T_FN && is_generic(expr_type)) {
    expr_val = create_generic_fn_binding(binding, expr, ctx);

    return expr_val;
  }

  if (expr_type->kind == T_FN && !is_coroutine_type(expr_type)) {

    if (expr->tag == AST_EXTERN_FN) {

      expr_val = create_lazy_extern_fn_binding(binding, expr, expr_type, NULL,
                                               ctx, module, builder);

    } else if (is_closure(expr_type)) {
      expr_val = codegen(expr, ctx, module, builder);
      create_fn_binding(binding, expr_type, expr_val, ctx, module, builder);
    } else {

      expr_val = create_fn_binding(binding, expr_type,
                                   codegen_fn(expr, ctx, module, builder), ctx,
                                   module, builder);
    }

    return expr_val;
  }

  if (expr->tag == AST_MODULE) {
    return codegen_inline_module(binding, expr, ctx, module, builder);
  }

  if (expr->tag == AST_IMPORT) {
    codegen_import(expr, binding, ctx, module, builder);
    return LLVMConstInt(LLVMInt32Type(), 1, 0);
  }

  expr_val = codegen(expr, ctx, module, builder);

  if (!expr_val) {
    print_type_err(expr->type);
    fprintf(stderr, "Error - could not compile value for binding to ");
    print_ast_err(binding);
    print_codegen_location();
    return NULL;
  }

  LLVMValueRef match_result = codegen_pattern_binding(
      binding, expr_val, expr_type, ctx, module, builder);

  if (match_result == NULL) {
    fprintf(stderr, "Error: codegen for pattern binding in let expression "
                    "failed\n");
    print_codegen_location();
    return NULL;
  }

  return expr_val;
}

LLVMValueRef codegen_let_expr(Ast *ast, JITLangCtx *outer_ctx,
                              LLVMModuleRef module, LLVMBuilderRef builder) {

  Ast *binding = ast->data.AST_LET.binding;

  JITLangCtx cont_ctx = *outer_ctx;

  if (ast->data.AST_LET.in_expr != NULL) {
    STACK_ALLOC_CTX_PUSH(fn_ctx, outer_ctx)
    cont_ctx = fn_ctx;
  }

  LLVMValueRef res = _codegen_let_expr(binding, ast->data.AST_LET.expr,
                                       &cont_ctx, module, builder);

  if (ast->data.AST_LET.in_expr != NULL) {
    res = codegen(ast->data.AST_LET.in_expr, &cont_ctx, module, builder);
    destroy_ctx(&cont_ctx);
  }
  return res;
}
