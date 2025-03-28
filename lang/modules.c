#include "./modules.h"
#include "./types/common.h"
#include "ht.h"
#include "input.h"
#include "serde.h"
#include "types/inference.h"
#include "types/type.h"
#include <stdlib.h>
#include <string.h>

ht module_registry;

void init_module_registry() { ht_init(&module_registry); }

Ast *create_module_from_root(Ast *ast_root) {

  ast_root = ast_lambda(NULL, ast_root);
  ast_root->tag = AST_MODULE;
  return ast_root;
}

bool is_module_record_ast(Ast *ast) {
  Type *rec_type = ast->data.AST_RECORD_ACCESS.record->md;
  if (rec_type->kind == T_CONS &&
      CHARS_EQ(rec_type->data.T_CONS.name, TYPE_NAME_MODULE)) {
    return true;
  }
  return false;
}

Ast *parse_module(const char *filename) {

  __import_current_dir = get_dirname(filename);

  Ast *prog = parse_input_script(filename);
  prog = create_module_from_root(prog);

  if (!prog) {
    return NULL;
  }

  TICtx ti_ctx = {.env = NULL, .scope = 0};

  // Open memory stream, passing pointers to buffer and length
  ti_ctx.err_stream = stderr;

  if (!infer(prog, &ti_ctx)) {
    return NULL;
  }

  if (!solve_program_constraints(prog, &ti_ctx)) {
    return NULL;
  }

  return prog;
}

Type *get_import_type(Ast *ast) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;
  YLCModule *mod = ht_get(&module_registry, file_path);
  if (mod) {
    return mod->type;
  }

  Ast *prev_root = ast_root;
  ast_root = NULL;
  Ast *module_ast = parse_module(file_path);
  ast_root = prev_root;
  Type *module_type = module_ast->md;

  YLCModule *registered_module = malloc(sizeof(YLCModule));
  *registered_module =
      (YLCModule){.type = deep_copy_type(module_ast->md), .ast = module_ast};

  // ht_init(&registered_module->generics);

  ht_set(&module_registry, file_path, registered_module);

  return module_ast->md;
}

YLCModule *get_imported_module(Ast *ast) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;

  YLCModule *mod = ht_get(&module_registry, file_path);
  return mod;
}

// int get_import_ref(Ast *ast, void **ref, Ast **module_ast) {
//   const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;
//
//   YLCModule *mod = ht_get(&module_registry, file_path);
//   if (!mod) {
//     return 0;
//   }
//
//   if (mod->ref) {
//     *ref = mod->ref;
//     *module_ast = NULL;
//   }
//
//   *ref = NULL;
//   *module_ast = mod->ast;
//
//   return 1;
// }

void set_import_ref(Ast *ast, void *ref) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;

  YLCModule *mod = ht_get(&module_registry, file_path);

  if (mod) {
    mod->ref = ref;
    ht_set(&module_registry, file_path, mod);
    return;
  }
  fprintf(stderr, "Error: no module found for %s\n", file_path);
}
