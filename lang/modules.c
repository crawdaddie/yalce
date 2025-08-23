#include "./modules.h"
#include "./types/common.h"
// #include "escape_analysis.h"
#include "ht.h"
#include "input.h"
#include "types/inference.h"
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

Ast *parse_module(const char *filename, TypeEnv *env) {

  char *old_import_current_dir = __import_current_dir;
  __import_current_dir = get_dirname(filename);

  Ast *prog = parse_input_script(filename);

  prog = create_module_from_root(prog);

  if (!prog) {
    return NULL;
  }

  // TICtx ti_ctx = {.env = NULL, .scope = 0};
  //
  // // Open memory stream, passing pointers to buffer and length
  // ti_ctx.err_stream = stderr;
  //
  // if (!infer(prog, &ti_ctx)) {
  //   return NULL;
  // }
  // *env = *ti_ctx.env;

  // escape_analysis(prog);

  __import_current_dir = old_import_current_dir;
  return prog;
}
TypeEnv *module_type_analysis(Ast *prog) {

  TICtx ti_ctx = {.subst = NULL, .scope = 0};

  ti_ctx.err_stream = stderr;

  if (!infer(prog, &ti_ctx)) {
    return NULL;
  }
  return NULL;
}

Type *get_import_type(Ast *ast) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;
  YLCModule *mod = ht_get(&module_registry, file_path);
  if (mod) {
    return mod->type;
  }

  Ast *prev_root = ast_root;
  ast_root = NULL;
  TypeEnv *env = malloc(sizeof(TypeEnv));
  Ast *module_ast = parse_module(file_path, env);
  TypeEnv *tm = module_type_analysis(module_ast);
  if (!tm) {
    return NULL;
  }
  *env = *tm;
  ast_root = prev_root;
  Type *module_type = module_ast->md;

  YLCModule *registered_module = malloc(sizeof(YLCModule));
  *registered_module = (YLCModule){
      .type = deep_copy_type(module_ast->md), .ast = module_ast, .env = env};

  ht_set(&module_registry, file_path, registered_module);

  return module_ast->md;
}

YLCModule *get_imported_module(Ast *ast) {
  const char *file_path = ast->data.AST_IMPORT.fully_qualified_name;

  YLCModule *mod = ht_get(&module_registry, file_path);
  mod->ast->data.AST_IMPORT.fully_qualified_name = file_path;
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
