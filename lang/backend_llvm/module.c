#include "./module.h"
#include "codegen.h"
#include "function.h"
#include "globals.h"
#include "modules.h"
#include "serde.h"
#include "symbols.h"
#include "tuple.h"
#include "types.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <stdlib.h>
#include <string.h>

LLVMValueRef ___codegen_module(const char *bind_chars, YLCModule *registry_ref,
                               JITLangCtx *ctx, LLVMModuleRef llvm_module_ref,
                               LLVMBuilderRef builder) {

  Ast *ast = registry_ref->ast;
  Type *module_type = registry_ref->type;

  LLVMTypeRef llvm_module_type =
      type_to_llvm_type(module_type, ctx->env, llvm_module_ref);

  LLVMValueRef module_struct = LLVMGetUndef(llvm_module_type);

  STACK_ALLOC_CTX_PUSH(module_ctx, ctx)
  int len = ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  Ast **stmts = ast->data.AST_LAMBDA.body->data.AST_BODY.stmts;

  for (int i = 0; i < len; i++) {
    Ast *stmt = stmts[i];
    if (stmt->tag == AST_LET) {
      Type *member_type = module_type->data.T_CONS.args[i];

      if (member_type->kind == T_FN && is_generic(member_type)) {
        JITLangCtx _mod_generic_ctx = *ctx;

        create_generic_fn_binding(stmt->data.AST_LET.binding,
                                  stmt->data.AST_LET.expr, &_mod_generic_ctx);

        continue;
      }

      LLVMValueRef member_val =
          codegen(stmt, &module_ctx, llvm_module_ref, builder);
      module_struct = LLVMBuildInsertValue(builder, module_struct, member_val,
                                           i, "module_member");
    }
  }

  return module_struct;
}

LLVMValueRef bind_module_chars(const char *chars, int chars_len,
                               Type *module_type, YLCModule *mod,
                               LLVMValueRef module_val, JITLangCtx *ctx,
                               LLVMModuleRef llvm_module_ref,
                               LLVMBuilderRef builder) {

  uint64_t id_hash = hash_string(chars, chars_len);

  LLVMTypeRef llvm_module_type =
      type_to_llvm_type(module_type, ctx->env, llvm_module_ref);

  JITSymbol *sym =
      new_symbol(STYPE_MODULE, module_type, module_val, llvm_module_type);
  mod->ref = sym;
  // sym->symbol_data.STYPE_MODULE

  codegen_set_global(sym, module_val, module_type, llvm_module_type, ctx,
                     llvm_module_ref, builder);

  ht_set_hash(ctx->frame->table, chars, id_hash, sym);
  return module_val;
}

LLVMValueRef bind_module(Ast *binding, Type *module_type,
                         LLVMValueRef module_val, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder) {

  const char *chars = binding->data.AST_IDENTIFIER.value;
  int chars_len = binding->data.AST_IDENTIFIER.length;
  // return bind_module_chars(chars, chars_len, module_type, module_val, ctx,
  //                          module, builder);
}

LLVMValueRef bind_parametrized_module(Ast *binding, Ast *module_ast,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return NULL;
}

LLVMValueRef codegen_module(Ast *ast, JITLangCtx *ctx,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder) {
  printf("codegen inline module\n");
  print_ast(ast);
  return NULL;
}

void add_module_generic(Ast *stmt, JITLangCtx *ctx, ht *generic_storage) {
  Ast *fn_ast = stmt->data.AST_LET.expr;
  Ast *binding = stmt->data.AST_LET.binding;

  JITSymbol *sym = create_generic_fn_symbol(fn_ast, ctx);

  const char *id_chars = binding->data.AST_IDENTIFIER.value;
  int id_len = binding->data.AST_IDENTIFIER.length;

  ht_set_hash(generic_storage, id_chars, hash_string(id_chars, id_len), sym);
}

LLVMValueRef _codegen_module(Ast *module_ast, LLVMTypeRef llvm_module_type,
                             JITLangCtx *ctx, ht *generic_storage,
                             LLVMModuleRef llvm_module_ref,
                             LLVMBuilderRef builder) {
  Type *module_type = module_ast->md;
  int len = module_ast->data.AST_LAMBDA.body->data.AST_BODY.len;
  Ast **stmts = module_ast->data.AST_LAMBDA.body->data.AST_BODY.stmts;

  LLVMValueRef mod_struct_val = LLVMGetUndef(llvm_module_type);

  for (int i = 0; i < len; i++) {
    Type *t = module_type->data.T_CONS.args[i];
    Ast *stmt = stmts[i];
    if (t->kind == T_FN && is_generic(t)) {
      add_module_generic(stmt, ctx, generic_storage);

      continue;
    }

    LLVMValueRef val = codegen(stmt, ctx, llvm_module_ref, builder);
    mod_struct_val = LLVMBuildInsertValue(builder, mod_struct_val, val, i, "");
  }
  return mod_struct_val;
}

LLVMValueRef codegen_inline_module(Ast *binding, Ast *module_ast,
                                   JITLangCtx *ctx,
                                   LLVMModuleRef llvm_module_ref,
                                   LLVMBuilderRef builder) {

  YLCModule _module = {
      .type = module_ast->md,
      .ast = module_ast,
  };
  YLCModule *module = &_module;
  JITSymbol *module_symbol;

  if (module->ast) {
    Type *module_type = module->type;
    Ast *module_ast = module->ast;

    module_symbol = malloc(sizeof(JITSymbol));
    module_symbol->type = STYPE_MODULE;
    module_symbol->symbol_type = module_type;
    ht_init(&module_symbol->symbol_data.STYPE_MODULE.generics);
    ht *generic_storage = &module_symbol->symbol_data.STYPE_MODULE.generics;

    LLVMTypeRef llvm_module_type =
        type_to_llvm_type(module_type, ctx->env, llvm_module_ref);

    LLVMValueRef module_struct = LLVMGetUndef(llvm_module_type);

    JITLangCtx *module_ctx = malloc(sizeof(JITLangCtx));
    *module_ctx = *ctx;
    ht table;
    ht_init(&table);
    StackFrame sf = {.table = &table, .next = module_ctx->frame};
    module_ctx->frame = &sf;
    module_ctx->stack_ptr = ctx->stack_ptr + 1;

    LLVMValueRef mod_struct_val =
        _codegen_module(module_ast, llvm_module_type, module_ctx,
                        generic_storage, llvm_module_ref, builder);
    module_symbol->val = mod_struct_val;
    module_symbol->llvm_type = llvm_module_type;
    module_symbol->symbol_data.STYPE_MODULE.generics = *generic_storage;
    module_symbol->symbol_data.STYPE_MODULE.ctx = module_ctx;

    // hti it = ht_iterator(generic_storage);
    // bool cont = ht_next(&it);
    // printf("generic members in mod:\n");
    // for (; cont; cont = ht_next(&it)) {
    //   const char *key = it.key;
    //   JITSymbol *t = it.value;
    //   printf("%s: ", key);
    //   print_type(t->symbol_type);
    // }

    const char *mod_binding = binding->data.AST_IDENTIFIER.value;
    int mod_binding_len = strlen(mod_binding);
    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);
    module->ref = module_symbol;
  }

  return module_symbol->val;
}
LLVMValueRef codegen_import(Ast *ast, JITLangCtx *ctx,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder) {

  YLCModule *module = get_imported_module(ast);
  JITSymbol *module_symbol;

  if (module->ref) {
    // TODO: if we have an alias eg import X as Y then rebind symbol
    module_symbol = module->ref;
    return module_symbol->val;
  }

  if (module->ast) {
    Type *module_type = module->type;
    Ast *module_ast = module->ast;

    module_symbol = malloc(sizeof(JITSymbol));
    module_symbol->type = STYPE_MODULE;
    module_symbol->symbol_type = module_type;
    ht_init(&module_symbol->symbol_data.STYPE_MODULE.generics);
    ht *generic_storage = &module_symbol->symbol_data.STYPE_MODULE.generics;

    LLVMTypeRef llvm_module_type =
        type_to_llvm_type(module_type, ctx->env, llvm_module_ref);

    LLVMValueRef module_struct = LLVMGetUndef(llvm_module_type);

    JITLangCtx *module_ctx = malloc(sizeof(JITLangCtx));
    *module_ctx = *ctx;
    ht table;
    ht_init(&table);
    StackFrame sf = {.table = &table, .next = module_ctx->frame};
    module_ctx->frame = &sf;
    module_ctx->stack_ptr = ctx->stack_ptr + 1;

    LLVMValueRef mod_struct_val =
        _codegen_module(module_ast, llvm_module_type, module_ctx,
                        generic_storage, llvm_module_ref, builder);
    module_symbol->val = mod_struct_val;
    module_symbol->llvm_type = llvm_module_type;
    module_symbol->symbol_data.STYPE_MODULE.generics = *generic_storage;
    module_symbol->symbol_data.STYPE_MODULE.ctx = module_ctx;

    // hti it = ht_iterator(generic_storage);
    // bool cont = ht_next(&it);
    // printf("generic members in mod:\n");
    // for (; cont; cont = ht_next(&it)) {
    //   const char *key = it.key;
    //   JITSymbol *t = it.value;
    //   printf("%s: ", key);
    //   print_type(t->symbol_type);
    // }

    const char *mod_binding = ast->data.AST_IMPORT.identifier;
    int mod_binding_len = strlen(mod_binding);
    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);
    module->ref = module_symbol;
  }

  return module_symbol->val;
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

  Type *member_type = record_type->data.T_CONS.args[member_idx];
  bool member_is_generic = is_generic(member_type);

  if (!member_is_generic) {
    LLVMValueRef val = codegen_tuple_access(member_idx, module_symbol->val,
                                            module_symbol->llvm_type, builder);
    return val;
  }

  if (member_is_generic && expected_member_type->kind == T_FN) {
    const char *member_name = member->data.AST_IDENTIFIER.value;
    int member_name_len = member->data.AST_IDENTIFIER.length;
    JITSymbol *generic_fn =
        ht_get_hash(&module_symbol->symbol_data.STYPE_MODULE.generics,
                    member_name, hash_string(member_name, member_name_len));

    LLVMValueRef specific_callable = get_specific_callable(
        generic_fn, expected_member_type, ctx, llvm_module_ref, builder);

    return specific_callable;
  }

  return NULL;
}
