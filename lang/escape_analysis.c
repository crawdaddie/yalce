#include "./escape_analysis.h"
#include "./arena_allocator.h"
#include "parse.h"
#include "serde.h"
#include "types/type.h"
#include <string.h>

static int32_t next_alloc_id;

DECLARE_ARENA_ALLOCATOR(ea, 512);

Allocation *create_alloc(const char *varname, Ast *alloc_site, int scope) {
  Allocation *alloc = ea_alloc(sizeof(Allocation));
  *alloc = (Allocation){
      .id = next_alloc_id++,
      .varname = varname,
      .alloc_site = alloc_site,
      .scope = scope,
      .escapes = false,
      .is_returned = false,
      .is_captured = false,
  };
  return alloc;
}

void print_allocs(Allocation *allocs) {
  while (allocs) {
    printf("id %d: ", allocs->id);
    if (allocs->varname) {
      printf(" [%s] ", allocs->varname);
    }
    print_ast(allocs->alloc_site);
    allocs = allocs->next;
  }
}

Allocation *allocations_extend(Allocation *allocs, Allocation *alloc) {
  if (!allocs) {
    return alloc;
  }
  if (!alloc) {
    return alloc;
  }
  Allocation *a = alloc;
  while (a->next != NULL) {
    a = a->next;
  }
  a->next = allocs;
  return alloc;
}

void ctx_add_allocation(EACtx *ctx, Allocation *alloc) {
  alloc->next = ctx->allocations;
  ctx->allocations = alloc;
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

void ctx_bind_allocations(EACtx *ctx, Ast *binding, Allocation *allocs) {
  if (!allocs) {
    return;
  }
  switch (binding->tag) {
  case AST_IDENTIFIER: {
    Allocation *b = ea_alloc(sizeof(Allocation));
    *b = *allocs;
    b->varname = binding->data.AST_IDENTIFIER.value;
    ctx_add_allocation(ctx, b);
  }
  default: {
  }
  }
}

Allocation *ea(Ast *ast, EACtx *ctx) {

  if (!ast) {
    return NULL;
  }
  Allocation *allocations = NULL;

  switch (ast->tag) {
  case AST_ARRAY:
  case AST_LIST: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      Ast *item = ast->data.AST_LIST.items + i;
      allocations = allocations_extend(allocations, ea(item, ctx));
    }
    Allocation *list_alloc = create_alloc(NULL, ast, ctx->scope);
    return allocations_extend(allocations, list_alloc);
  }
  case AST_TUPLE: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      allocations = allocations_extend(allocations,
                                       ea(ast->data.AST_LIST.items + i, ctx));
    }
    // TUPLE doesn't allocate anything however it can expose allocations from
    // its members
    return allocations;
  }
  case AST_INT:
  case AST_DOUBLE:
  case AST_CHAR:
  case AST_BOOL: {
    break;
  }
  case AST_STRING: {
    Allocation *str_alloc = create_alloc(NULL, ast, ctx->scope);
    return allocations_extend(allocations, str_alloc);
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
    Allocation *ret_alloc;

    EACtx lambda_ctx = *ctx;
    lambda_ctx.allocations = NULL;
    lambda_ctx.scope++;
    if (ast->data.AST_LAMBDA.body->tag != AST_BODY) {
      Ast *stmt = ast->data.AST_LAMBDA.body;
      lambda_ctx.is_return_stmt = true;
      ret_alloc = ea(stmt, &lambda_ctx);
    } else {
      Ast *body = ast->data.AST_LAMBDA.body;
      Ast *stmt;
      AST_LIST_ITER(body->data.AST_BODY.stmts, ({
                      stmt = l->ast;
                      if (i == body->data.AST_BODY.len - 1) {
                        lambda_ctx.is_return_stmt = true;
                        ret_alloc = ea(stmt, &lambda_ctx);
                      } else {
                        ea(stmt, &lambda_ctx);
                      }
                    }));
    }

    for (Allocation *a = lambda_ctx.allocations; a; a = a->next) {
      EscapeMeta *ea_meta = malloc(sizeof(EscapeMeta));
      *ea_meta = (EscapeMeta){.status = EA_STACK_ALLOC, .id = a->id};
      a->alloc_site->ea_md = ea_meta;
    }

    if (ret_alloc) {
      for (Allocation *r = ret_alloc; r; r = r->next) {
        EscapeMeta *ea_meta = malloc(sizeof(EscapeMeta));
        *ea_meta = (EscapeMeta){.status = EA_HEAP_ALLOC, .id = r->id};
        r->alloc_site->ea_md = ea_meta;
      }
    }

    if (is_closure(ast->type)) {
      Allocation *closure_alloc = create_alloc(NULL, ast, ctx->scope);
      return allocations_extend(allocations, closure_alloc);
    }

    return NULL;
  }

  case AST_BODY: {
    Ast *stmt;
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    stmt = l->ast;
                    allocations =
                        allocations_extend(allocations, ea(stmt, ctx));
                  }));
    return allocations;
  }

  case AST_APPLICATION: {

    // ea(ast->data.AST_APPLICATION.function, ctx);

    // for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    //   ea(ast->data.AST_APPLICATION.args + i, ctx);
    // }

    break;
  }
  case AST_LOOP:
  case AST_LET: {
    allocations = ea(ast->data.AST_LET.expr, ctx);

    if (ast->data.AST_LET.in_expr) {
      EACtx *let_ctx = ctx;
      let_ctx->scope++;
      ctx_bind_allocations(let_ctx, ast->data.AST_LET.binding, allocations);
      return ea(ast->data.AST_LET.in_expr, let_ctx);
    }
    ctx_bind_allocations(ctx, ast->data.AST_LET.binding, allocations);
    return allocations;
  }

  case AST_MATCH: {
    ea(ast->data.AST_MATCH.expr, ctx);

    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      Allocation *branch_allocs =
          ea(ast->data.AST_MATCH.branches + (2 * i) + 1, ctx);
      allocations = allocations_extend(allocations, branch_allocs);
    }
    return allocations;
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
    Allocation *found_alloc = ctx_find_allocation(ctx, varname);
    return found_alloc;
  }

  default: {
    break;
  }
  }
  return NULL;
}

void escape_analysis(Ast *prog) {
  WITH_ARENA_ALLOCATOR(ea, ({
                         EACtx ctx = {};
                         ea(prog, &ctx);
                       }));
}
