#include "./modules.h"
#include "./types/common.h"
#include "./types/inference.h"
// #include "escape_analysis.h"
#include "ht.h"
#include "serde.h"
#include "types/type.h"
#include "types/type_ser.h"
#include <regex.h>
#include <stdlib.h>
#include <string.h>

void print_constraints(Constraint *constraints);
void *type_error(Ast *node, const char *fmt, ...);

ht module_registry;

void init_module_registry() { ht_init(&module_registry); }

Ast *create_module_from_root(Ast *ast_root) {

  ast_root = ast_lambda(NULL, ast_root);
  ast_root->tag = AST_MODULE;
  return ast_root;
}

bool is_module_ast(Ast *ast) {
  Type *t = ast->type;
  return t && t->kind == T_MODULE;
}

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

YLCModule *init_import(YLCModule *mod) {
  ParsingContext _pctx = pctx;
  Ast *mod_ast = parse_input_script(mod->path);
  pctx = _pctx;

  mod_ast = ast_lambda(NULL, mod_ast);
  mod_ast->tag = AST_MODULE;
  custom_binops_t *custom_binops = pctx.custom_binops;
  mod->ast = mod_ast;
  mod->custom_binops = custom_binops;

  TICtx mod_ctx = {.custom_binops = custom_binops};
  Type *mod_type = infer(mod->ast, &mod_ctx);
  if (!mod_type || mod_type->kind != T_MODULE) {
    fprintf(stderr, "Error: failed to infer module %s as T_MODULE\n",
            mod->path);
    mod->type = NULL;
    mod->env = NULL;
    return mod;
  }

  mod->type = mod_type;
  mod->env = mod_type->data.T_MODULE.env;
  return mod;
}

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
                  .env = NULL,
                  .path = key};

  ht_set(&module_registry, key, new_module);
  return false; // success
}
