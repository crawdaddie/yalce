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

LLVMValueRef __codegen_module(Ast *module_ast, LLVMTypeRef llvm_module_type,
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
  module_ctx->stack_ptr = ctx->stack_ptr + 1;
  return module_ctx;
}

#define PRINT_MODULE_AT_IMPORT

LLVMValueRef compile_module(JITSymbol *module_symbol, Ast *module_ast,
                            LLVMModuleRef llvm_module_ref,
                            LLVMBuilderRef builder) {
  JITLangCtx *ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;
  codegen_lambda_body(module_ast, ctx, llvm_module_ref, builder);

#ifdef PRINT_MODULE_AT_IMPORT
  hti it = ht_iterator(ctx->frame->table);
  bool cont = ht_next(&it);
  printf("module top-level:\n");
  for (; cont; cont = ht_next(&it)) {
    const char *key = it.key;
    JITSymbol *t = it.value;
    printf("%s: ", key);
    print_type(t->symbol_type);
  }
#endif
}

JITSymbol *create_module_symbol(Type *module_type, Ast *module_ast,
                                JITLangCtx *ctx,
                                LLVMModuleRef llvm_module_ref) {
  int mod_len = module_type->data.T_CONS.num_args;

  JITSymbol *module_symbol = malloc(sizeof(JITSymbol) + mod_len * sizeof(int));

  // for (int i = 0; i < mod_len; i++) {
  //   module_symbol->symbol_data.STYPE_MODULE.map.val_map[i] = -1;
  //   printf("mod val map %d: %d\n", i,
  //          module_symbol->symbol_data.STYPE_MODULE.map.val_map[i]);
  // }

  module_symbol->type = STYPE_MODULE;
  module_symbol->symbol_type = module_type;

  JITLangCtx *module_ctx = heap_alloc_ctx(ctx);
  module_symbol->symbol_data.STYPE_MODULE.ctx = module_ctx;
  return module_symbol;
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
    int mod_len = module_type->data.T_CONS.num_args;
    Ast *module_ast = module->ast;

    module_symbol =
        create_module_symbol(module_type, module_ast, ctx, llvm_module_ref);
    compile_module(module_symbol, module_ast, llvm_module_ref, builder);

    const char *mod_binding = binding->data.AST_IDENTIFIER.value;
    int mod_binding_len = binding->data.AST_IDENTIFIER.length;

    ht_set_hash(ctx->frame->table, mod_binding,
                hash_string(mod_binding, mod_binding_len), module_symbol);

    module->ref = module_symbol;
  }

  return module_symbol->val;
}

JITSymbol *codegen_import(Ast *ast, Ast *binding, JITLangCtx *ctx,
                          LLVMModuleRef llvm_module_ref,
                          LLVMBuilderRef builder) {

  YLCModule *module = get_imported_module(ast);
  JITSymbol *module_symbol;

  if (module->ref) {
    // TODO: if we have an alias eg import X as Y then rebind symbol
    module_symbol = module->ref;
  } else if (module->ast) {
    Type *module_type = module->type;
    int mod_len = module_type->data.T_CONS.num_args;
    Ast *module_ast = module->ast;

    module_symbol =
        create_module_symbol(module_type, module_ast, ctx, llvm_module_ref);
    compile_module(module_symbol, module_ast, llvm_module_ref, builder);
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

  JITSymbol *member_symbol =
      lookup_id_ast(member, module_symbol->symbol_data.STYPE_MODULE.ctx);

  return member_symbol->val;
}
