#include "./escape_analysis.h"
#include "serde.h"
#include "types/common.h"

void ea(Ast *ast, AECtx *ctx) {

  if (!ast) {
    return;
  }

  switch (ast->tag) {
  case AST_TUPLE:
  case AST_ARRAY:
  case AST_LIST: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      ea(ast->data.AST_LIST.items + i, ctx);
    }

    break;
  }

  case AST_INT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_BOOL:
  case AST_STRING: {
    break;
  }

  case AST_FMT_STRING: {

    int arity = ast->data.AST_LIST.len;
    for (int i = 0; i < arity; i++) {
      Ast *member = ast->data.AST_LIST.items + i;
      ea(member, ctx);
    }

    break;
  }

  case AST_LAMBDA: {
    ea(ast->data.AST_LAMBDA.body, ctx);
    break;
  }

  case AST_BODY: {
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      ea(ast->data.AST_BODY.stmts[i], ctx);
    }
    break;
  }

  case AST_APPLICATION: {

    ea(ast->data.AST_APPLICATION.function, ctx);

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      ea(ast->data.AST_APPLICATION.args + i, ctx);
    }

    break;
  }
  case AST_LOOP:
  case AST_LET: {
    ea(ast->data.AST_LET.expr, ctx);

    if (ast->data.AST_LET.in_expr) {
      ea(ast->data.AST_LET.in_expr, ctx);
    }

    break;
  }

  case AST_MATCH: {
    ea(ast->data.AST_MATCH.expr, ctx);
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      ea(ast->data.AST_MATCH.branches + (2 * i), ctx);
      ea(ast->data.AST_MATCH.branches + (2 * i) + 1, ctx);
    }

    break;
  }

  case AST_MATCH_GUARD_CLAUSE: {
    ea(ast->data.AST_MATCH_GUARD_CLAUSE.guard_expr, ctx);
    break;
  }

  case AST_YIELD: {
    ea(ast->data.AST_YIELD.expr, ctx);
    break;
  }

  default: {
    break;
  }
  }
}
void escape_analysis(Ast *prog, AECtx *ctx) { return ea(prog, ctx); }
