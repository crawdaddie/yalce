#ifndef _LANG_MODULES_H
#define _LANG_MODULES_H
#include "ht.h"
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
extern ht module_registry;
void init_module_registry();

typedef struct {
  Type *type;
  Ast *ast;
  void *ref;
  TypeEnv *env;
} YLCModule;

Type *get_import_type(Ast *ast);

int get_import_ref(Ast *ast, void **ref, Ast **module_ast);

void set_import_ref(Ast *ast, void *ref);

YLCModule *get_imported_module(Ast *ast);

bool is_module_ast(Ast *ast);
#endif
