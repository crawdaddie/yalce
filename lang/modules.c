#include "./modules.h"
#include "./types/common.h"
// #include "escape_analysis.h"
#include "ht.h"
#include "types/type.h"
#include <regex.h>
#include <stdlib.h>
#include <string.h>

ht module_registry;

void init_module_registry() { ht_init(&module_registry); }

Ast *create_module_from_root(Ast *ast_root) {

  ast_root = ast_lambda(NULL, ast_root);
  ast_root->tag = AST_MODULE;
  return ast_root;
}

bool is_module_ast(Ast *ast) {
  Type *t = ast->md;
  return t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, TYPE_NAME_MODULE);
}

Type *get_import_type(Ast *ast) { return NULL; }

YLCModule *get_imported_module(Ast *ast) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;

  YLCModule *mod = ht_get(&module_registry, file_path);
  mod->ast->data.AST_IMPORT.fully_qualified_name = file_path;
  return mod;
}

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

YLCModule *get_module(const char *key) { return ht_get(&module_registry, key); }

bool module_exists(const char *key) {
  return ht_get(&module_registry, key) != NULL;
}

bool register_module_ast(const char *key, Ast *module_ast) {
  YLCModule *new_module = malloc(sizeof(YLCModule));
  if (!new_module) {
    return true; // allocation failed
  }

  *new_module =
      (YLCModule){.type = NULL, // Will be filled during type inference
                  .ast = module_ast,
                  .ref = NULL,
                  .env = NULL};

  ht_set(&module_registry, key, new_module);
  return false; // success
}
