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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int concrete_fn_name_counter = 0;

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

static char *copy_string_len(const char *chars, size_t len) {
  if (!chars) {
    return NULL;
  }
  char *copy = malloc(len + 1);
  if (!copy) {
    return NULL;
  }
  memcpy(copy, chars, len);
  copy[len] = '\0';
  return copy;
}

static char *copy_llvm_value_name(LLVMValueRef value) {
  if (!value) {
    return NULL;
  }
  size_t len = 0;
  const char *name = LLVMGetValueName2(value, &len);
  return copy_string_len(name, len);
}

static char *make_unique_function_name(const char *binding_name) {
  if (!binding_name) {
    binding_name = "anonymous";
  }

  int len = snprintf(NULL, 0, "__ylc_fn.%s.%d", binding_name,
                     concrete_fn_name_counter++);
  if (len < 0) {
    return NULL;
  }

  char *name = malloc((size_t)len + 1);
  if (!name) {
    return NULL;
  }

  snprintf(name, (size_t)len + 1, "__ylc_fn.%s.%d", binding_name,
           concrete_fn_name_counter - 1);
  return name;
}

static void assign_top_level_function_jit_name(JITSymbol *sym,
                                               LLVMValueRef value,
                                               const char *binding_name,
                                               JITLangCtx *ctx) {
  if (!sym || !value || !LLVMIsAFunction(value)) {
    return;
  }

  if (ctx && ctx->stack_ptr == 0) {
    char *name = make_unique_function_name(binding_name);
    if (name) {
      LLVMSetValueName2(value, name, strlen(name));
      sym->jit_name = name;
      return;
    }
  }

  sym->jit_name = copy_llvm_value_name(value);
}

static void specialize_ast_types_for_codegen(Ast *ast, JITLangCtx *ctx) {
  if (!ast) {
    return;
  }

  if (ast->type) {
    ast->type = specialize_type_for_codegen(ast->type, ctx);
  }

  switch (ast->tag) {
  case AST_BODY:
    AST_LIST_ITER(ast->data.AST_BODY.stmts,
                  ({ specialize_ast_types_for_codegen(l->ast, ctx); }));
    break;
  case AST_LET:
    specialize_ast_types_for_codegen(ast->data.AST_LET.binding, ctx);
    specialize_ast_types_for_codegen(ast->data.AST_LET.expr, ctx);
    specialize_ast_types_for_codegen(ast->data.AST_LET.in_expr, ctx);
    break;
  case AST_APPLICATION:
    specialize_ast_types_for_codegen(ast->data.AST_APPLICATION.function, ctx);
    for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      specialize_ast_types_for_codegen(ast->data.AST_APPLICATION.args + i, ctx);
    }
    break;
  case AST_LAMBDA:
  case AST_MODULE:
    AST_LIST_ITER(ast->data.AST_LAMBDA.params,
                  ({ specialize_ast_types_for_codegen(l->ast, ctx); }));
    specialize_ast_types_for_codegen(ast->data.AST_LAMBDA.body, ctx);
    break;
  case AST_MATCH:
    specialize_ast_types_for_codegen(ast->data.AST_MATCH.expr, ctx);
    for (size_t i = 0; i < ast->data.AST_MATCH.len * 2; i++) {
      specialize_ast_types_for_codegen(ast->data.AST_MATCH.branches + i, ctx);
    }
    break;
  case AST_MATCH_GUARD_CLAUSE:
    specialize_ast_types_for_codegen(ast->data.AST_MATCH_GUARD_CLAUSE.test_expr,
                                     ctx);
    specialize_ast_types_for_codegen(
        ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx);
    break;
  case AST_LIST:
  case AST_TUPLE:
    for (size_t i = 0; i < ast->data.AST_LIST.len; i++) {
      specialize_ast_types_for_codegen(ast->data.AST_LIST.items + i, ctx);
    }
    break;
  case AST_RECORD_ACCESS:
    specialize_ast_types_for_codegen(ast->data.AST_RECORD_ACCESS.record, ctx);
    specialize_ast_types_for_codegen(ast->data.AST_RECORD_ACCESS.member, ctx);
    break;
  case AST_YIELD:
    specialize_ast_types_for_codegen(ast->data.AST_YIELD.expr, ctx);
    break;
  case AST_UNOP:
    specialize_ast_types_for_codegen(ast->data.AST_UNOP.expr, ctx);
    break;
  case AST_RANGE_EXPRESSION:
    specialize_ast_types_for_codegen(ast->data.AST_RANGE_EXPRESSION.from, ctx);
    specialize_ast_types_for_codegen(ast->data.AST_RANGE_EXPRESSION.to, ctx);
    break;
  default:
    break;
  }
}

static JITSymbol *clone_symbol(JITSymbol *sym) {
  if (!sym) {
    return NULL;
  }

  JITSymbol *copy = malloc(sizeof(JITSymbol));
  memcpy(copy, sym, sizeof(JITSymbol));
  return copy;
}

static bool is_type_module_param(Ast *param, Ast *annotation) {
  return param && param->tag == AST_IDENTIFIER && annotation == NULL;
}

typedef enum BindingKind {
  BIND_ARRAY_FN,
  BIND_PARTIAL_CLOSURE,
  BIND_COROUTINE,
  BIND_COROUTINE_CONSTRUCTOR,
  BIND_GENERIC_FN,
  BIND_GENERIC_MODULE,
  BIND_SPECIALIZED_MODULE,
  BIND_CONCRETE_FN,
  BIND_MODULE,
  BIND_IMPORT,
  BIND_VALUE,
} BindingKind;

// Prefer the finalized env binding type when classifying a let-bound symbol.
static Type *resolve_binding_type(Ast *binding, Ast *expr, JITLangCtx *ctx) {
  Type *binding_type = specialize_type_for_codegen(expr->type, ctx);
  if (binding->tag != AST_IDENTIFIER) {
    return binding_type;
  }

  TypeEnv *entry =
      lookup_type_ref(ctx->env, binding->data.AST_IDENTIFIER.value);
  if (entry && entry->type) {
    Type *env_type = specialize_type_for_codegen(entry->type, ctx);
    if (binding_type && !is_generic(binding_type) &&
        (!env_type || is_generic(env_type))) {
      return binding_type;
    }
    return env_type;
  }

  return binding_type;
}

// Rebind one identifier name to another existing symbol in the current scope.
static JITSymbol *bind_alias_if_possible(Ast *binding, Ast *expr,
                                         JITLangCtx *ctx) {
  if (binding->tag != AST_IDENTIFIER || expr->tag != AST_IDENTIFIER) {
    return NULL;
  }

  JITSymbol *sym = lookup_id_ast(expr, ctx);
  if (!sym) {
    return NULL;
  }

  sym = clone_symbol(sym);
  if (!sym) {
    return NULL;
  }

  const char *chars = binding->data.AST_IDENTIFIER.value;
  int len = binding->data.AST_IDENTIFIER.length;
  ht_set_hash(ctx->frame->table, chars, hash_string(chars, len), sym);
  return sym;
}

// Classify a let-binding before dispatching to the concrete emission path.
static BindingKind classify_binding(Ast *expr, Type *expr_type,
                                    Type *binding_type) {
  if (is_coroutine_constructor_type(expr_type)) {
    return BIND_COROUTINE_CONSTRUCTOR;
  }
  if (is_coroutine_type(binding_type)) {
    return BIND_COROUTINE;
  }

  if (expr->tag == AST_MODULE && expr->data.AST_LAMBDA.len > 0) {
    return BIND_GENERIC_MODULE;
  }

  if (expr->tag == AST_APPLICATION && is_module(binding_type)) {
    return BIND_SPECIALIZED_MODULE;
  }

  if (binding_type->kind == T_FN && is_generic(binding_type)) {
    return BIND_GENERIC_FN;
  }

  if (binding_type->kind == T_FN) {
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
    LLVMValueRef expr_val = codegen_create_closure(expr, ctx, module, builder);
    if (!expr_val) {
      expr_val = codegen(expr, ctx, module, builder);
    }
    create_fn_binding(binding, binding_type, expr_val, ctx, module, builder);
    return expr_val;
  }

  if (expr->tag == AST_APPLICATION &&
      expr->data.AST_APPLICATION.args->tag == AST_LAMBDA) {
    // Special case: eg let s = @Audio fn () -> ... ;; -- decorated lambda
    return codegen(expr, ctx, module, builder);
  }

  if (expr->tag == AST_APPLICATION &&
      expr->data.AST_APPLICATION.args->tag == AST_EXTERN_FN) {

    LLVMValueRef func = codegen(expr, ctx, module, builder);
    create_fn_binding(binding, binding_type, func, ctx, module, builder);
    return func;
  }

  specialize_ast_types_for_codegen(expr, ctx);
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

LLVMValueRef rematerialize_function_symbol(JITSymbol *sym, JITLangCtx *ctx,
                                           LLVMModuleRef module) {
  if (!sym) {
    return NULL;
  }

  if (!sym->symbol_type || is_closure(sym->symbol_type)) {
    return sym->val;
  }

  if (!sym->jit_name) {
    return sym->val;
  }

  LLVMValueRef existing = LLVMGetNamedFunction(module, sym->jit_name);
  if (existing) {
    return existing;
  }

  int fn_len = fn_type_args_len(sym->symbol_type);
  LLVMTypeRef fn_type = NULL;
  if (is_coroutine_constructor_type(sym->symbol_type)) {
    LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
    LLVMTypeRef generic_ptr =
        LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
    fn_type = codegen_coro_fn_type(generic_ptr, sym->symbol_type, fn_len, ctx,
                                   module);
  } else {
    fn_type = codegen_fn_type(NULL, sym->symbol_type, fn_len, ctx, module);
  }
  if (!fn_type) {
    return NULL;
  }

  LLVMValueRef decl = LLVMAddFunction(module, sym->jit_name, fn_type);
  LLVMSetLinkage(decl, LLVMExternalLinkage);
  return decl;
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
    return codegen_get_global(chars, sym, ctx, module, builder);

  case STYPE_FUNCTION:
    return rematerialize_function_symbol(sym, ctx, module);

  case STYPE_LAZY_EXTERN_FUNCTION:
    sym->val = codegen_extern_fn(
        sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast, ctx, module, builder);
    return sym->val;

  case STYPE_GENERIC_FUNCTION: {
    Type *expected_type = specialize_type_for_codegen(ast->type, ctx);
    // printf("load generic fn id: ");
    // print_ast(ast);
    // print_type(expected_type);
    // print_type(sym->symbol_type);
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

// Register a parametrized module binding without forcing specialization yet.
LLVMValueRef create_generic_module_binding(Ast *binding, Ast *module_ast,
                                           JITLangCtx *ctx) {
  JITSymbol *sym =
      new_symbol(STYPE_GENERIC_MODULE, module_ast->type, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_MODULE.ast = module_ast;
  sym->symbol_data.STYPE_GENERIC_MODULE.stack_ptr = ctx->stack_ptr;
  sym->symbol_data.STYPE_GENERIC_MODULE.stack_frame = ctx->frame;
  sym->symbol_data.STYPE_GENERIC_MODULE.type_env = ctx->env;
  sym->symbol_data.STYPE_GENERIC_MODULE.specific_fns = NULL;
  int len = module_ast->data.AST_LAMBDA.len;
  ModuleParamKind *param_kinds =
      len > 0 ? malloc(sizeof(ModuleParamKind) * len) : NULL;
  int num_type_params = 0;
  int num_value_params = 0;
  AstList *param = module_ast->data.AST_LAMBDA.params;
  AstList *annotation = module_ast->data.AST_LAMBDA.type_annotations;
  for (int i = 0; i < len && param; i++, param = param->next) {
    Ast *ann_ast = annotation ? annotation->ast : NULL;
    ModuleParamKind kind = is_type_module_param(param->ast, ann_ast)
                               ? MODULE_PARAM_TYPE
                               : MODULE_PARAM_VALUE;
    if (param_kinds) {
      param_kinds[i] = kind;
    }
    if (kind == MODULE_PARAM_TYPE) {
      num_type_params++;
    } else {
      num_value_params++;
    }
    if (annotation) {
      annotation = annotation->next;
    }
  }
  sym->symbol_data.STYPE_GENERIC_MODULE.param_kinds = param_kinds;
  sym->symbol_data.STYPE_GENERIC_MODULE.num_type_params = num_type_params;
  sym->symbol_data.STYPE_GENERIC_MODULE.num_value_params = num_value_params;
  install_identifier_symbol(binding, sym, ctx);
  return NULL;
}

// Register a concrete function value in the current frame.
LLVMValueRef create_fn_binding(Ast *binding, Type *fn_type, LLVMValueRef fn,
                               JITLangCtx *ctx, LLVMModuleRef module,
                               LLVMBuilderRef builder) {

  JITSymbol *sym = new_symbol(STYPE_FUNCTION, fn_type, fn, NULL);
  if (binding && binding->tag == AST_IDENTIFIER) {
    assign_top_level_function_jit_name(sym, fn,
                                       binding->data.AST_IDENTIFIER.value, ctx);
  }
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
  sym->jit_name = copy_string_len(expr->data.AST_EXTERN_FN.fn_name.chars,
                                  expr->data.AST_EXTERN_FN.fn_name.length);
  sym->symbol_data.STYPE_LAZY_EXTERN_FUNCTION.ast = expr;
  install_identifier_symbol(binding, sym, ctx);
  return fn;
}

LLVMValueRef create_closure_symbol(Ast *binding, Ast *expr, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  printf("create closure symbol\n");
  print_ast(expr);
  print_type(expr->type);
  JITSymbol *sym = create_generic_fn_symbol(expr, ctx);
  install_identifier_symbol(binding, sym, ctx);
  return NULL;
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

  JITSymbol *alias_sym = bind_alias_if_possible(binding, expr, ctx);
  if (alias_sym) {
    return alias_sym->val;
  }

  switch (classify_binding(expr, expr_type, binding_type)) {
  case BIND_ARRAY_FN:
    return emit_array_fn_binding(binding, expr, expr_type, ctx, module,
                                 builder);
  case BIND_COROUTINE_CONSTRUCTOR:
    return create_coroutine_constructor_symbol(binding, expr, binding_type, ctx,
                                               module, builder);
  case BIND_COROUTINE:
    return create_coroutine_symbol(binding, expr, binding_type, ctx, module,
                                   builder);
  case BIND_GENERIC_FN:
    return create_generic_fn_binding(binding, expr, ctx);
  case BIND_GENERIC_MODULE:
    return create_generic_module_binding(binding, expr, ctx);

  case BIND_SPECIALIZED_MODULE:
    return specialize_and_bind_module(binding, expr, binding_type, ctx, module,
                                      builder);

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
