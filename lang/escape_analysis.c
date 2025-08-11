#include "./escape_analysis.h"
#include "types/type.h"
#include <stdlib.h>
#include <string.h>

static int32_t next_alloc_id;

void ctx_add_allocation(EACtx *ctx, const char *varname, Ast *alloc_site) {
  Allocation *alloc = malloc(sizeof(Allocation));
  *alloc = (Allocation){.id = next_alloc_id++,
                        .varname = varname,
                        .alloc_site = alloc_site,
                        .escapes = false,
                        .is_returned = false,
                        .is_captured = false,
                        .next = ctx->allocations};
  ctx->allocations = alloc;

  printf("Added allocation %d for variable '%s'\n", alloc->id, varname);
}

Allocation *ctx_find_allocation(EACtx *ctx, const char *varname) {
  if (!ctx->allocations) {
    return NULL;
  }
  for (Allocation *a = ctx->allocations; a; a = a->next) {
    if (strcmp(a->varname, varname) == 0) {
      return a;
    }
  }
  return NULL;
}

void ea(Ast *ast, EACtx *ctx) {

  if (!ast) {
    return;
  }

  switch (ast->tag) {
  case AST_ARRAY:
  case AST_LIST: {

    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
    }
    break;
  }

  case AST_TUPLE: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
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

    EACtx lambda_ctx = *ctx;
    lambda_ctx.scope++;
    if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
      Ast *stmt = ast->data.AST_LAMBDA.body;
      lambda_ctx.is_return_stmt = true;
      ea(stmt, &lambda_ctx);
    } else {
      Ast *body = ast->data.AST_LAMBDA.body;
      Ast *stmt;
      for (int i = 0; i < body->data.AST_BODY.len; i++) {
        stmt = body->data.AST_BODY.stmts[i];

        if (i == body->data.AST_BODY.len - 1) {
          lambda_ctx.is_return_stmt = true;
        }
        ea(stmt, &lambda_ctx);
      }
    }
    break;
  }

  case AST_BODY: {

    Ast *stmt;
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      stmt = ast->data.AST_BODY.stmts[i];
      ea(stmt, ctx);
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

    Type *t = ast->data.AST_LET.expr->md;

    if (ast->data.AST_LET.in_expr) {
      ea(ast->data.AST_LET.in_expr, ctx);
    }

    break;
  }

  case AST_MATCH: {
    ea(ast->data.AST_MATCH.expr, ctx);
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      ea(ast->data.AST_MATCH.branches + (2 * i), ctx);
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
  case AST_IDENTIFIER: {
    const char *varname = ast->data.AST_IDENTIFIER.value;

    break;
  }

  default: {
    break;
  }
  }
  return;
}
void escape_analysis(Ast *prog) {
  EACtx ctx = {};
  ea(prog, &ctx);
}
