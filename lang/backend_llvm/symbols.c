#include "backend_llvm/symbols.h"

#include "./coroutines/coroutines.h"
#include "adt.h"
#include "application.h"
#include "binding.h"
#include "closures.h"
#include "codegen.h"
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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void mark_invariant(LLVMValueRef load_inst) {
  // unsigned kind = LLVMGetMDKindID("invariant.load", 14);
  // LLVMValueRef md = LLVMMDNodeInContext(LLVMGetGlobalContext(), NULL, 0);
  // LLVMSetMetadata(load_inst, kind, md);
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef create_lazy_extern_fn_binding(Ast *binding, Ast *expr,
                                           Type *fn_type, LLVMValueRef fn,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder);

typedef enum BindingKind {
  BIND_ARRAY_FN,
  BIND_PARTIAL_CLOSURE,
  BIND_COROUTINE,
  BIND_GENERIC_FN,
  BIND_CONCRETE_FN,
  BIND_MODULE,
  BIND_IMPORT,
  BIND_VALUE,
} BindingKind;

// Prefer the finalized env binding type when classifying a let-bound symbol.
static Type *resolve_binding_type(Ast *binding, Ast *expr, JITLangCtx *ctx) {
  Type *binding_type = expr->type;
  if (binding->tag != AST_IDENTIFIER) {
    return binding_type;
  }

  TypeEnv *entry =
      lookup_type_ref(ctx->env, binding->data.AST_IDENTIFIER.value);
  if (entry && entry->type) {
    return entry->type;
  }

  return binding_type;
}

// Rebind one identifier name to another existing symbol in the current scope.
static LLVMValueRef bind_alias_if_possible(Ast *binding, Ast *expr,
                                           JITLangCtx *ctx) {
  if (binding->tag != AST_IDENTIFIER || expr->tag != AST_IDENTIFIER) {
    return NULL;
  }

  JITSymbol *sym = lookup_id_ast(expr, ctx);
  if (!sym) {
    return NULL;
  }

  const char *chars = binding->data.AST_IDENTIFIER.value;
  int len = binding->data.AST_IDENTIFIER.length;
  ht_set_hash(ctx->frame->table, chars, hash_string(chars, len), sym);
  return sym->val;
}

// Detect the special array accessor closure shape produced by `array_at`.
static Type *classify_array_accessor_type(Ast *expr) {
  if (expr->tag != AST_APPLICATION) {
    return NULL;
  }

  if (expr->data.AST_APPLICATION.function->tag != AST_IDENTIFIER) {
    return NULL;
  }

  if (strcmp("array_at",
             expr->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value) !=
      0) {
    return NULL;
  }

  Type *arr = expr->data.AST_APPLICATION.args->type;
  if (arr->kind == T_CONS && is_array_type(arr) &&
      arr->data.T_CONS.args[0]->kind == T_FN) {
    return arr->data.T_CONS.args[0];
  }

  return NULL;
}

// Classify a let-binding before dispatching to the concrete emission path.
static BindingKind classify_binding(Ast *expr, Type *expr_type,
                                    Type *binding_type) {
  Type *array_fn_type = classify_array_accessor_type(expr);
  if (array_fn_type) {
    return is_generic(array_fn_type) ? BIND_GENERIC_FN : BIND_ARRAY_FN;
  }

  if (expr->tag == AST_APPLICATION && expr->type->kind == T_FN) {

    return BIND_PARTIAL_CLOSURE;
  }

  if (is_coroutine_constructor_type(binding_type)) {
    return BIND_COROUTINE;
  }

  if (binding_type->kind == T_FN && is_generic(binding_type)) {
    return BIND_GENERIC_FN;
  }

  if (binding_type->kind == T_FN && !is_coroutine_type(binding_type)) {
    return BIND_CONCRETE_FN;
  }

  if (expr->tag == AST_MODULE) {
    return BIND_MODULE;
  }

  if (expr->tag == AST_IMPORT) {
    return BIND_IMPORT;
  }

  return BIND_VALUE;
}

// Materialize a non-generic array accessor binding as a callable symbol.
static LLVMValueRef emit_array_fn_binding(Ast *binding, Ast *expr,
                                          Type *expr_type, JITLangCtx *ctx,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder) {
  return create_fn_binding(binding, expr_type,
                           codegen(expr, ctx, module, builder), ctx, module,
                           builder);
}

// Emit concrete function-like bindings, including externs and closures.
static LLVMValueRef emit_function_binding(Ast *binding, Ast *expr,
                                          Type *binding_type, JITLangCtx *ctx,
                                          LLVMModuleRef module,
                                          LLVMBuilderRef builder) {
  if (expr->tag == AST_EXTERN_FN) {
    return create_lazy_extern_fn_binding(binding, expr, binding_type, NULL, ctx,
                                         module, builder);
  }

  if (is_closure(binding_type)) {
    LLVMValueRef expr_val = codegen(expr, ctx, module, builder);
    create_fn_binding(binding, binding_type, expr_val, ctx, module, builder);
    return expr_val;
  }

  if (expr->tag == AST_APPLICATION &&
      expr->data.AST_APPLICATION.args->tag == AST_LAMBDA) {
    // Special case: eg let s = @Audio fn () -> ... ;; -- decorated lambda
    return codegen(expr, ctx, module, builder);
  }

  return create_fn_binding(binding, binding_type,
                           codegen_fn(expr, ctx, module, builder), ctx, module,
                           builder);
}

// Lower a plain value binding and apply any pattern destructuring side effects.
static LLVMValueRef emit_value_binding(Ast *binding, Ast *expr, Type *expr_type,
                                       JITLangCtx *ctx, LLVMModuleRef module,
                                       LLVMBuilderRef builder) {
  LLVMValueRef expr_val = codegen(expr, ctx, module, builder);

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

// Allocate and initialize a backend symbol record.
JITSymbol *new_symbol(symbol_type type_tag, Type *symbol_type, LLVMValueRef val,
                      LLVMTypeRef llvm_type) {

  JITSymbol *sym = malloc(sizeof(JITSymbol));
  memset(sym, 0, sizeof(JITSymbol));
  sym->type = type_tag;
  sym->symbol_type = symbol_type;
  sym->val = val;
  sym->llvm_type = llvm_type;
  // TODO: if it's a symbol do I need to create a storage class???

  return sym;
}

// Resolve identifier-like AST nodes to an existing backend symbol.
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

// Fall back to enum / ADT / builtin identifier materialization when no symbol
// exists.
static LLVMValueRef codegen_identifier_fallback(Ast *ast, const char *chars,
                                                JITLangCtx *ctx,
                                                LLVMModuleRef module,
                                                LLVMBuilderRef builder) {
  Type *enum_type = env_lookup(ctx->env, chars);
  if (!enum_type) {
    enum_type = lookup_builtin_type(chars);
  }
  if (!enum_type) {
    fprintf(stderr,
            "codegen identifier failed enum '%s' not found in scope %d %s:%d\n",
            chars, ctx->stack_ptr, __FILE__, __LINE__);
    print_codegen_location();
    return NULL;
  }

  if (is_simple_enum(enum_type)) {
    return codegen_simple_enum_member(enum_type, chars, ctx, module, builder);
  }

  if (strcmp(chars, "None") == 0) {
    LLVMTypeRef llvm_type = type_to_llvm_type(ast->type, ctx, module);

    if (!llvm_type) {
      print_location(ast);
      fprintf(stderr, "Option type not found\n");
      return NULL;
    }

    LLVMValueRef v = LLVMGetUndef(llvm_type);
    v = LLVMBuildInsertValue(builder, v, LLVMConstInt(LLVMInt8Type(), 1, 0), 0,
                             "insert None tag");
    return v;
  }

  return codegen_adt_member(enum_type, chars, ctx, module, builder);
}

// Load or realize a resolved symbol according to its storage and symbol kind.
static LLVMValueRef load_identifier_symbol(Ast *ast, const char *chars,
                                           JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  switch (sym->type) {
  case STYPE_TOP_LEVEL_VAR:
    return codegen_get_global(chars, sym, module, builder);

  case STYPE_FUNCTION:
    return sym->val;

  case STYPE_LAZY_EXTERN_FUNCTION:
    if (sym->val) {
      return sym->val;
    }

    sym->val = codegen_extern_fn(
        sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast, ctx, module, builder);
    return sym->val;

  case STYPE_GENERIC_FUNCTION: {
    Type *expected_type = specialize_type_for_codegen(ast->type, ctx);
    return get_specific_callable(sym, expected_type, ctx, module, builder);
  }

  case STYPE_LOCAL_VAR:
    if (sym->storage != NULL) {
      Type *loaded_type =
          sym->symbol_type ? specialize_type_for_codegen(sym->symbol_type, ctx)
                           : specialize_type_for_codegen(ast->type, ctx);
      LLVMTypeRef llvm_type = type_to_llvm_type(loaded_type, ctx, module);
      LLVMValueRef load_inst =
          LLVMBuildLoad2(builder, llvm_type, sym->storage, "load pointer");
      mark_invariant(load_inst);
      return load_inst;
    }

    return sym->val;

  case STYPE_VARIANT_TYPE:
    return codegen_adt_member(ast->type, chars, ctx, module, builder);

  default:
    return sym->val;
  }
}

// Entry point for identifier codegen: resolve a symbol first, then fall back.
LLVMValueRef codegen_identifier(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  const char *chars = ast->data.AST_IDENTIFIER.value;

  JITSymbol *sym = lookup_id_ast(ast, ctx);

  if (!sym) {
    return codegen_identifier_fallback(ast, chars, ctx, module, builder);
  }

  return load_identifier_symbol(ast, chars, sym, ctx, module, builder);
}

// Build the deferred metadata used to specialize a generic function on demand.
JITSymbol *create_generic_fn_symbol(Ast *fn_ast, JITLangCtx *ctx) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, fn_ast->type, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.ast = fn_ast;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = ctx->stack_ptr;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_frame = ctx->frame;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.type_env = ctx->env;
  return sym;
}

// Install a symbol for an identifier binding in the current frame.
static void install_identifier_symbol(Ast *binding, JITSymbol *sym,
                                      JITLangCtx *ctx) {
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;
  ht_set_hash(ctx->frame->table, id_chars, hash_string(id_chars, id_len), sym);
}

// Register a generic function binding without forcing specialization yet.
LLVMValueRef create_generic_fn_binding(Ast *binding, Ast *fn_ast,
                                       JITLangCtx *ctx) {
  JITSymbol *sym = create_generic_fn_symbol(fn_ast, ctx);
  install_identifier_symbol(binding, sym, ctx);
  return NULL;
}

// Register a concrete function value in the current frame.
LLVMValueRef create_fn_binding(Ast *binding, Type *fn_type, LLVMValueRef fn,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, fn, NULL);
  install_identifier_symbol(binding, sym, ctx);
  return fn;
}

// Register an extern function whose LLVM declaration is emitted lazily on first
// use.
LLVMValueRef create_lazy_extern_fn_binding(Ast *binding, Ast *expr,
                                           Type *fn_type, LLVMValueRef fn,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {

  JITSymbol *sym = malloc(sizeof(JITSymbol));
  memset(sym, 0, sizeof(JITSymbol));
  sym->type = STYPE_LAZY_EXTERN_FUNCTION;
  sym->symbol_type = fn_type;
  sym->val = NULL;
  sym->llvm_type = NULL;
  sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast = expr;
  install_identifier_symbol(binding, sym, ctx);
  return fn;
}

LLVMValueRef __handle_yield_boundary_crossing_binding(Ast *binding, Ast *expr,
                                                      JITLangCtx *ctx,
                                                      LLVMModuleRef module,
                                                      LLVMBuilderRef builder);
// Lower the right-hand side of a let-binding and install the resulting
// symbol/value.
LLVMValueRef _codegen_let_expr(Ast *binding, Ast *expr, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  if (ctx->coro_ctx) {
    __handle_yield_boundary_crossing_binding(binding, expr, ctx, module,
                                             builder);
  }
  Type *expr_type = expr->type;
  Type *binding_type = resolve_binding_type(binding, expr, ctx);

  LLVMValueRef alias_val = bind_alias_if_possible(binding, expr, ctx);
  if (alias_val) {
    return alias_val;
  }

  switch (classify_binding(expr, expr_type, binding_type)) {
  case BIND_ARRAY_FN:
    return emit_array_fn_binding(binding, expr, expr_type, ctx, module,
                                 builder);
  case BIND_PARTIAL_CLOSURE:
    return create_closure_symbol(binding, expr, ctx, module, builder);

  case BIND_COROUTINE:
    return create_coroutine_symbol(binding, expr, binding_type, ctx, module,
                                   builder);
  case BIND_GENERIC_FN:
    return create_generic_fn_binding(binding, expr, ctx);

  case BIND_CONCRETE_FN:
    return emit_function_binding(binding, expr, binding_type, ctx, module,
                                 builder);
  case BIND_MODULE:
    return codegen_inline_module(binding, expr, ctx, module, builder);
  case BIND_IMPORT:
    codegen_import(expr, binding, ctx, module, builder);
    return LLVMConstInt(LLVMInt32Type(), 1, 0);
  case BIND_VALUE:
    return emit_value_binding(binding, expr, expr_type, ctx, module, builder);
  }

  return NULL;
}

// Lower a let-expression, creating a child codegen scope for `in` expressions.
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
