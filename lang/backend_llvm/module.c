#include "./module.h"
#include "function.h"
#include "function_extern.h"
#include "globals.h"
#include "modules.h"
#include "serde.h"
#include "symbols.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include "types.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

void add_module_generic(Ast *stmt, JITLangCtx *ctx, ht *generic_storage) {
  Ast *fn_ast = stmt->data.AST_LET.expr;
  Ast *binding = stmt->data.AST_LET.binding;

  JITSymbol *sym = create_generic_fn_symbol(fn_ast, ctx);
  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(generic_storage, id_chars, hash_string(id_chars, id_len), sym);
}

bool is_exportable(Ast *stmt) {
  if (stmt->tag == AST_TYPE_DECL) {
    return false;
  }
  if (stmt->tag == AST_LET && stmt->data.AST_LET.in_expr != NULL) {
    return false;
  }
  return true;
}

JITLangCtx *heap_alloc_ctx(JITLangCtx *ctx) {
  char *mem = malloc(sizeof(JITLangCtx) + sizeof(ht) + sizeof(StackFrame));
  JITLangCtx *module_ctx = (JITLangCtx *)mem;
  mem += sizeof(JITLangCtx);
  *module_ctx = *ctx;
  ht *table = (ht *)mem;
  mem += sizeof(ht);
  ht_init(table);
  StackFrame *sf = (StackFrame *)mem;
  mem += sizeof(StackFrame);
  *sf = (StackFrame){.table = table, .next = module_ctx->frame};
  module_ctx->frame = sf;
  // TODO: is this legit - incrementing stack ptr means non-function module
  // values get stack alloced and forgotten? module_ctx->stack_ptr =
  // ctx->stack_ptr + 1;
  module_ctx->stack_ptr = ctx->stack_ptr;
  return module_ctx;
}

// #define PRINT_MODULE_AT_IMPORT

LLVMValueRef compile_module(JITSymbol *module_symbol, Ast *module_ast,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder) {
  JITLangCtx *ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;
  codegen_lambda_body(module_ast, ctx, llvm_module_ref, builder);

#ifdef PRINT_MODULE_AT_IMPORT
  hti it = ht_iterator(ctx->frame->table);
  bool cont = ht_next(&it);
  printf("\nmodule top-level\n");
  for (; cont; cont = ht_next(&it)) {
    const char *key = it.key;
    JITSymbol *t = it.value;
    printf("%s: ", key);
    print_type(t->symbol_type);
  }
#endif

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}
LLVMValueRef create_constructor_module(Ast *trait, JITLangCtx *ctx,
                                       LLVMModuleRef llvm_module_ref,
                                       LLVMBuilderRef builder) {

  ObjString type_name = trait->data.AST_TRAIT_IMPL.type;

  Ast binding = (Ast){AST_IDENTIFIER,
                      .data = {.AST_IDENTIFIER = {.value = type_name.chars}}};

  Ast *module_ast = trait->data.AST_TRAIT_IMPL.impl;

  YLCModule _module = {
      .type = module_ast->type,
      .ast = module_ast,
  };

  const char *mod_binding = binding.data.AST_IDENTIFIER.value;
  int mod_binding_len = strlen(mod_binding);
  YLCModule *module = &_module;
  JITSymbol *module_symbol;

  Type *module_type = module->type;
  Type *underlying = module_type->data.T_CONS.args[0];

  underlying = fn_return_type(underlying);

  module_symbol =
      create_module_symbol(module_type, NULL, module_ast, ctx, llvm_module_ref);

  compile_module(module_symbol, module_ast, llvm_module_ref, builder);

  ht_set_hash(ctx->frame->table, mod_binding,
              hash_string(mod_binding, mod_binding_len), module_symbol);
  if (!is_pointer_type(underlying)) {
    char *canonical_name = underlying->data.T_CONS.name;
    ht_set_hash(ctx->frame->table, canonical_name,
                hash_string(canonical_name, strlen(canonical_name)),
                module_symbol);
  }

  module->ref = module_symbol;

  Type *out_type = env_lookup(ctx->env, type_name.chars);
  if (!out_type) {
    out_type = lookup_builtin_type(type_name.chars);
  }

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

JITSymbol *create_module_symbol(Type *module_type, TypeEnv *module_type_env,
                                Ast *module_ast, JITLangCtx *ctx,
                                LLVMModuleRef llvm_module_ref) {
  int mod_len = module_type->data.T_CONS.num_args;

  JITSymbol *module_symbol = malloc(sizeof(JITSymbol) + mod_len * sizeof(int));

  module_symbol->type = STYPE_MODULE;
  module_symbol->symbol_type = module_type;

  JITLangCtx *module_ctx = heap_alloc_ctx(ctx);
  module_ctx->env = module_type_env;
  module_symbol->symbol_data.STYPE_MODULE.ctx = module_ctx;
  return module_symbol;
}

LLVMValueRef codegen_inline_module(Ast *binding, Ast *module_ast,
                                   JITLangCtx *ctx,
                                   LLVMModuleRef llvm_module_ref,
                                   LLVMBuilderRef builder) {

  YLCModule _module = {
      .type = module_ast->type,
      .ast = module_ast,
  };

  YLCModule *module = &_module;
  JITSymbol *module_symbol;

  if (module->ast) {
    Type *module_type = module->type;
    int mod_len = module_type->data.T_CONS.num_args;
    Ast *module_ast = module->ast;

    module_symbol = create_module_symbol(module_type, NULL, module_ast, ctx,
                                         llvm_module_ref);

    compile_module(module_symbol, module_ast, llvm_module_ref, builder);

    const char *mod_binding = binding->data.AST_IDENTIFIER.value;
    int mod_binding_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);

    module->ref = module_symbol;
  }

  // return module_symbol->val;
  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

const char *module_path = NULL;

JITSymbol *codegen_import(Ast *ast, Ast *binding, JITLangCtx *ctx,
                          LLVMModuleRef llvm_module_ref,
                          LLVMBuilderRef builder) {

  YLCModule *module = get_imported_module(ast);

  const char *prev_mod_path = module_path;
  module_path = ast->data.AST_IMPORT.fully_qualified_name;

  JITSymbol *module_symbol;

  if (module->ref) {
    // TODO: if we have an alias eg import X as Y then rebind symbol
    module_symbol = module->ref;
  } else if (module->ast) {
    Type *module_type = module->type;
    int mod_len = module_type->data.T_CONS.num_args;
    Ast *module_ast = module->ast;

    TypeEnv *module_type_env = module->env;
    module_symbol = create_module_symbol(module_type, module_type_env,
                                         module_ast, ctx, llvm_module_ref);

    compile_module(module_symbol, module_ast, llvm_module_ref, builder);
    module->ref = module_symbol;
  }

  if (ast->data.AST_IMPORT.import_all) {
    JITLangCtx *module_ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;

    hti it = ht_iterator(module_ctx->frame->table);
    bool cont = ht_next(&it);
    for (; cont; cont = ht_next(&it)) {
      const char *key = it.key;
      JITSymbol *sym = it.value;
      int len = strlen(key);
      ht_set_hash(ctx->frame->table, key, hash_string(key, len), sym);
    }
  } else {
    const char *mod_binding;
    int mod_binding_len;
    if (binding) {
      mod_binding = binding->data.AST_IDENTIFIER.value;
      mod_binding_len = strlen(mod_binding);

    } else {
      mod_binding = ast->data.AST_IMPORT.identifier;
      mod_binding_len = strlen(mod_binding);
    }

    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);

    module->ref = module_symbol;
  }

  module_path = prev_mod_path;
  return module_symbol;
}

LLVMValueRef codegen_module_access(Ast *record_ast, Type *record_type,
                                   int member_idx, Ast *member,
                                   Type *expected_member_type, JITLangCtx *ctx,
                                   LLVMModuleRef llvm_module_ref,
                                   LLVMBuilderRef builder) {

  JITSymbol *module_symbol = lookup_id_ast(record_ast, ctx);
  if (!module_symbol) {
    fprintf(stderr, "Error: module %s not found in scope %d\n",
            record_ast->data.AST_IDENTIFIER.value, ctx->stack_ptr);
    return NULL;
  }

  JITSymbol *sym =
      lookup_id_ast(member, module_symbol->symbol_data.STYPE_MODULE.ctx);

  if (!sym) {
    fprintf(stderr, "Error: module member %s not found in module context\n",
            member->data.AST_IDENTIFIER.value);
    return NULL;
  }

  if (sym->type == STYPE_GENERIC_FUNCTION) {
    return get_specific_callable(sym, expected_member_type,
                                 module_symbol->symbol_data.STYPE_MODULE.ctx,
                                 llvm_module_ref, builder);
  }

  if (sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    return instantiate_extern_fn_sym(sym, ctx, llvm_module_ref, builder);
  }

  if (sym->type == STYPE_TOP_LEVEL_VAR) {
    const char *member_name = member->data.AST_IDENTIFIER.value;
    return codegen_get_global(member_name, sym, llvm_module_ref, builder);
  }

  return sym->val;
}

// Specialize a parametrized module application (e.g. `Set (fn x -> ...)`) and
// install the resulting STYPE_MODULE under the binding name.
LLVMValueRef specialize_and_bind_module(Ast *binding, Ast *app,
                                        Type *binding_type, JITLangCtx *ctx,
                                        LLVMModuleRef llvm_module_ref,
                                        LLVMBuilderRef builder) {
  Ast *fn = app->data.AST_APPLICATION.function;
  JITSymbol *gen_sym = lookup_id_ast(fn, ctx);
  if (!gen_sym || gen_sym->type != STYPE_GENERIC_MODULE) {
    fprintf(stderr, "Error: expected generic module for specialization\n");
    return NULL;
  }

  // Build the expected function type from the application: arg1_type ->
  // arg2_type -> ... -> result_type. Unifying this with the module's generic
  // type produces the substitution that resolves the module's type variables
  // (e.g. a = Int).
  int nargs = app->data.AST_APPLICATION.len;
  if (nargs <= 0) {
    fprintf(stderr, "Error: module application has no arguments\n");
    return NULL;
  }

  Type *exp_fn_type = deep_copy_type(app->type);
  for (int i = nargs - 1; i >= 0; i--) {
    Type *arg_t = deep_copy_type(app->data.AST_APPLICATION.args[i].type);
    Type *fn = t_alloc(sizeof(Type));
    *fn = (Type){T_FN, {.T_FN = {arg_t, exp_fn_type}}};
    exp_fn_type = fn;
  }

  Type *exp_copy = deep_copy_type(exp_fn_type);
  Type *sym_copy = deep_copy_type(gen_sym->symbol_type);
  TICtx ti_ctx = {};
  unify(exp_copy, sym_copy, &ti_ctx);
  Subst *subst = solve_constraints(ti_ctx.constraints);

  TypeEnv *spec_env =
      create_env_from_subst(gen_sym->symbol_data.STYPE_GENERIC_MODULE.type_env,
                           subst);

  Type *module_type = fn_return_type(gen_sym->symbol_type);
  module_type = apply_subst_to_type(subst, deep_copy_type(module_type));

  JITSymbol *module_symbol =
      create_module_symbol(module_type, spec_env,
                           gen_sym->symbol_data.STYPE_GENERIC_MODULE.ast, ctx,
                           llvm_module_ref);

  JITLangCtx *module_ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;
  module_ctx->type_subst = subst;
  module_ctx->env = spec_env;

  // Bind the module's parameters from the application's arguments so the
  // body can reference them (e.g. `hash` in `module hash: (a -> Uint64) -> ...`).
  // The arguments are compiled under a specialized context (with the
  // substitution applied) so that type-driven builtins like `asbytes` resolve
  // the module's type variable (e.g. `a = Int`).
  Ast *module_ast = gen_sym->symbol_data.STYPE_GENERIC_MODULE.ast;
  Type *param_type_cursor = gen_sym->symbol_type;
  int arg_i = 0;
  AST_LIST_ITER(module_ast->data.AST_LAMBDA.params, ({
    if (arg_i >= app->data.AST_APPLICATION.len) break;
    if (param_type_cursor->kind != T_FN) break;
    Ast *param_ast = l->ast;
    Ast *arg_ast = app->data.AST_APPLICATION.args + arg_i;

    LLVMValueRef arg_val = codegen(arg_ast, module_ctx, llvm_module_ref, builder);
    if (!arg_val) {
      fprintf(stderr, "Error: failed to compile module parameter argument\n");
    }

    Type *p_type = specialize_type_for_codegen(param_type_cursor->data.T_FN.from,
                                               module_ctx);
    bind_fn_param(arg_val, p_type, param_ast, ctx, module_ctx, llvm_module_ref,
                  builder);

    param_type_cursor = param_type_cursor->data.T_FN.to;
    arg_i++;
  }));

  compile_module(module_symbol, module_ast, llvm_module_ref, builder);

  const char *mod_binding = binding->data.AST_IDENTIFIER.value;
  int mod_binding_len = binding->data.AST_IDENTIFIER.length;
  ht_set_hash(ctx->frame->table, mod_binding,
              hash_string(mod_binding, mod_binding_len), module_symbol);

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}
