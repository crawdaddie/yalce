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
  Type *t = ast->md;
  return t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, TYPE_NAME_MODULE);
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

Type *infer_module_import(Ast *ast, TICtx *ctx) {

  AstList *type_annotations = ast->data.AST_LAMBDA.type_annotations;
  AstList *params = ast->data.AST_LAMBDA.params;

  // printf("infer parametrized module\n");
  if (ast->data.AST_LAMBDA.len > 0) {

    int i = 0;
    for (AstList *p = params; p; i++, p = p->next,
                 type_annotations = type_annotations ? type_annotations->next
                                                     : NULL) {
      // Ast *param = p->ast;
      // print_ast(param);
      // if (type_annotations && type_annotations->ast) {
      //   printf("constraint: ");
      //   print_ast(type_annotations->ast);
      // }
    }
  }

  Ast *body_ast;
  AstList *stmt_list;
  int len;

  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    // Single statement - create a temporary AstList node
    AstList *single_stmt = t_alloc(sizeof(AstList));
    single_stmt->ast = ast->data.AST_LAMBDA.body;
    single_stmt->next = NULL;
    stmt_list = single_stmt;
    len = 1;
  } else {
    body_ast = ast->data.AST_LAMBDA.body;
    stmt_list = body_ast->data.AST_BODY.stmts;
    len = body_ast->data.AST_BODY.len;
  }

  TypeEnv *env_start = ctx->env;

  Ast *stmt;
  Type **member_types = t_alloc(sizeof(Type *) * len);
  const char **names = t_alloc(sizeof(char *) * len);

  int i = 0;
  for (AstList *current = stmt_list; current != NULL;
       current = current->next, i++) {
    stmt = current->ast;
    Type *t = infer(stmt, ctx);

    if (!((stmt->tag == AST_LET) || (stmt->tag == AST_TYPE_DECL) ||
          (stmt->tag == AST_IMPORT) || (stmt->tag == AST_TRAIT_IMPL))) {

      // TODO: iter over stmts and only count the 'exportable members'
      member_types[i] = NULL;
    } else {
      member_types[i] = t;

      if (stmt->tag == AST_TYPE_DECL) {
        names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;

      } else if (stmt->tag == AST_IMPORT) {

        names[i] = stmt->data.AST_IMPORT.identifier;
      } else {
        names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;
      }

      if (!t) {
        printf("could not infer module member\n");
        print_ast_err(stmt);
        return NULL;
      }
    }
  }

  Type *module_struct_type =
      create_cons_type(TYPE_NAME_MODULE, len, member_types);

  module_struct_type->data.T_CONS.names = names;

  // TODO: do we need to keep module env scope from being pushed up
  // ctx->env = env;

  return module_struct_type;
}

Type *infer_inline_module(Ast *ast, TICtx *ctx) {

  AstList *type_annotations = ast->data.AST_LAMBDA.type_annotations;
  AstList *params = ast->data.AST_LAMBDA.params;

  // printf("infer parametrized module\n");
  if (ast->data.AST_LAMBDA.len > 0) {

    int i = 0;
    for (AstList *p = params; p; i++, p = p->next,
                 type_annotations = type_annotations ? type_annotations->next
                                                     : NULL) {
      // Ast *param = p->ast;
      // print_ast(param);
      // if (type_annotations && type_annotations->ast) {
      //   printf("constraint: ");
      //   print_ast(type_annotations->ast);
      // }
    }
  }

  Ast *body_ast;
  AstList *stmt_list;
  int len;

  if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    // Single statement - create a temporary AstList node
    AstList *single_stmt = t_alloc(sizeof(AstList));
    single_stmt->ast = ast->data.AST_LAMBDA.body;
    single_stmt->next = NULL;
    stmt_list = single_stmt;
    len = 1;
  } else {
    body_ast = ast->data.AST_LAMBDA.body;
    stmt_list = body_ast->data.AST_BODY.stmts;
    len = body_ast->data.AST_BODY.len;
  }

  TICtx module_ctx = *ctx;
  TypeEnv *env_start = module_ctx.env;

  Ast *stmt;
  Type **member_types = t_alloc(sizeof(Type *) * len);
  const char **names = t_alloc(sizeof(char *) * len);

  int i = 0;
  for (AstList *current = stmt_list; current != NULL; current = current->next) {
    stmt = current->ast;
    if (!((stmt->tag == AST_LET) || (stmt->tag == AST_TYPE_DECL) ||
          (stmt->tag == AST_IMPORT) || (stmt->tag == AST_TRAIT_IMPL))) {
      return type_error(stmt,
                        "Please only have let statements and type declarations "
                        "in a module\n");
      return NULL;
    }

    Type *t = infer(stmt, &module_ctx);
    member_types[i] = t;

    if (stmt->tag == AST_TYPE_DECL) {
      names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;

    } else if (stmt->tag == AST_IMPORT) {

      names[i] = stmt->data.AST_IMPORT.identifier;
    } else {
      names[i] = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;
    }

    if (!t) {
      print_ast_err(stmt);
      return NULL;
    }
    i++;
  }

  TypeEnv *env = module_ctx.env;

  Type *module_struct_type =
      create_cons_type(TYPE_NAME_MODULE, len, member_types);

  module_struct_type->data.T_CONS.names = names;

  // TODO: do we need to keep module env scope from being pushed up
  // ctx->env = env;

  return module_struct_type;
}

YLCModule *init_import(YLCModule *mod) {
  Ast *mod_ast = parse_input_script(mod->path);

  mod_ast = ast_lambda(NULL, mod_ast);
  mod_ast->tag = AST_MODULE;
  custom_binops_t *custom_binops = pctx.custom_binops;
  mod->ast = mod_ast;
  mod->custom_binops = custom_binops;

  TICtx mod_ctx = {};
  Type *mod_struct = infer_module_import(mod->ast, &mod_ctx);
  mod->type = mod_struct;
  mod->env = mod_ctx.env;
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

Type *infer_module_access(Ast *ast, Type *rec_type, const char *member_name,
                          TICtx *ctx) {
  Type *type = NULL;

  for (int i = 0; i < rec_type->data.T_CONS.num_args; i++) {
    if (CHARS_EQ(rec_type->data.T_CONS.names[i], member_name)) {
      type = rec_type->data.T_CONS.args[i];
      ast->data.AST_RECORD_ACCESS.index = i;
      break;
    }

    // if (type == NULL) {
    //   fprintf(stderr, "Error: %s not found in module %s\n", member_name,
    //           ast->data.AST_RECORD_ACCESS.record->data.AST_IDENTIFIER.value);
    //   return NULL;
    // }
  }

  if (is_generic(type)) {
    Type *gen = generalize(type, NULL);
    type = instantiate(gen, ctx);
  }
  return type;
}
Type *get_import_type(Ast *ast) { return NULL; }
