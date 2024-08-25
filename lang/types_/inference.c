#include "types/inference.h"
#include "common.h"
#include "format_utils.h"
#include "serde.h"
#include "types/type.h"
#include <stdlib.h>
#include <string.h>

// forward decl
int infer(TypeEnv **env, Ast *ast);

// Global variables
static int type_var_counter = 0;

// Main type inference function
// traverse Ast nodes and set each node Ast->md to the inferred type
//
int infer(TypeEnv **env, Ast *ast) {
  Type type;
  ast->md = type;
  switch (ast->tag) {
  case AST_BODY: {

    TypeEnv **current_env = env;
    for (size_t i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      infer(current_env, stmt);
    }
    break;
  }
  }
}

int infer_ast(TypeEnv **env, Ast *ast) {
  if (infer(env, ast)) {
    fprintf(stderr, "Type inference failed\n");
    return 1;
  }

  return 0;
}
