#include "./escape_analysis.h"
#include "serde.h"
#include "types/common.h"
#include "types/type.h"
#include <stdlib.h>
#include <string.h>
void *_ealloc(size_t size) { return malloc(size); }

bool type_needs_alloc(Type *t) { return is_list_type(t) || is_array_type(t); }

static uint32_t __memory_id = 0;
uint32_t next_mem_id() {
  uint32_t id = __memory_id;
  __memory_id++;
  return id;
}

typedef struct MemoryUseList {
  uint32_t id;
  struct MemoryUseList *next;
} MemoryUseList;

void print_memory_list(MemoryUseList *l) {
  if (!l) {
    return;
  }

  printf("[ ");
  while (l) {
    printf("%d, ", l->id);
    l = l->next;
  }
  printf("]");
}
EscapesEnv *escapes_add(EscapesEnv *env, const char *name, Ast *node,
                        uint32_t id) {

  EscapesEnv *new = _ealloc(sizeof(EscapesEnv));

  *new = (EscapesEnv){
      .varname = name,
      .expr = node,
      .id = id,
      .next = env,
  };
  return new;
}

EscapesEnv *escapes_find(EscapesEnv *env, const char *name) {
  while (env) {
    if (CHARS_EQ(env->varname, name)) {
      return env;
    }
    env = env->next;
  }
  return NULL;
}

EscapesEnv *escapes_find_by_id(EscapesEnv *env, uint32_t id) {
  while (env) {
    if (env->id == id) {
      return env;
    }
    env = env->next;
  }
  return NULL;
}

MemoryUseList *list_extend_left(MemoryUseList *old, MemoryUseList *new) {
  if (!old) {
    return new;
  }
  if (!new) {
    return old;
  }
  MemoryUseList *l = new;
  while (l->next) {
    l = l->next;
  }
  l->next = old;
  return new;
}

MemoryUseList *ea(Ast *ast, EACtx *ctx) {
  MemoryUseList *mem_ids = NULL;

  if (!ast) {
    return NULL;
  }

  switch (ast->tag) {
  case AST_ARRAY:
  case AST_LIST: {

    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      mem_ids =
          list_extend_left(mem_ids, ea(ast->data.AST_LIST.items + i, ctx));
    }
    MemoryUseList *container_mem = _ealloc(sizeof(MemoryUseList));
    *container_mem = (MemoryUseList){next_mem_id(), NULL};
    mem_ids = list_extend_left(mem_ids, container_mem);
    break;
  }

  case AST_TUPLE: {
    for (int i = 0; i < ast->data.AST_LIST.len; i++) {
      mem_ids =
          list_extend_left(mem_ids, ea(ast->data.AST_LIST.items + i, ctx));
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

    MemoryUseList *escapees = ea(ast->data.AST_LAMBDA.body, &lambda_ctx);

    for (MemoryUseList *esc = escapees; esc != NULL; esc = esc->next) {
      uint32_t mem_id = esc->id;
      EscapesEnv *env = escapes_find_by_id(lambda_ctx.env, mem_id);
      if (env) {
        EscapeMeta *ea_md = malloc(sizeof(EscapeMeta));
        *ea_md = (EscapeMeta){.status = EA_HEAP_ALLOC};
        env->expr->ea_md = ea_md;
      } else {

        // EscapeMeta *ea_md = malloc(sizeof(EscapeMeta));
        // *ea_md = (EscapeMeta){.status = EA_STACK_ALLOC};
        // lambda_ctx.env =
        // ctx->env = escapes_add(ctx->env, );
        // env->expr->ea_md = ea_md;
      }
    }
    break;
  }

  case AST_BODY: {
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      mem_ids = ea(ast->data.AST_BODY.stmts[i], ctx);
    }
    break;
  }

  case AST_APPLICATION: {

    ea(ast->data.AST_APPLICATION.function, ctx);

    for (int i = 0; i < ast->data.AST_APPLICATION.len; i++) {
      ea(ast->data.AST_APPLICATION.args + i, ctx);
    }

    if (ast->md && type_needs_alloc(ast->md)) {
      MemoryUseList *container_mem = _ealloc(sizeof(MemoryUseList));
      *container_mem = (MemoryUseList){next_mem_id(), NULL};
      mem_ids = list_extend_left(mem_ids, container_mem);
    }
    break;
  }
  case AST_LOOP:
  case AST_LET: {
    MemoryUseList *expr_ids = ea(ast->data.AST_LET.expr, ctx);

    Type *t = ast->data.AST_LET.expr->md;
    // printf("ast let\n");
    // print_type(t);
    // printf("binding type needs alloc %d\n", type_needs_alloc(t));

    if (type_needs_alloc(t) &&
        ast->data.AST_LET.binding->tag == AST_IDENTIFIER && expr_ids) {
      // printf("ast let type needs alloc\n");

      ctx->env = escapes_add(
          ctx->env, ast->data.AST_LET.binding->data.AST_IDENTIFIER.value,
          ast->data.AST_LET.expr,
          expr_ids->id // use first id as the 'containing' id
      );
    }
    mem_ids = expr_ids;

    if (ast->data.AST_LET.in_expr) {
      mem_ids = ea(ast->data.AST_LET.in_expr, ctx);
    }

    break;
  }

  case AST_MATCH: {
    ea(ast->data.AST_MATCH.expr, ctx);
    for (int i = 0; i < ast->data.AST_MATCH.len; i++) {
      ea(ast->data.AST_MATCH.branches + (2 * i), ctx);
      mem_ids = list_extend_left(
          mem_ids, ea(ast->data.AST_MATCH.branches + (2 * i) + 1, ctx));
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
    EscapesEnv *ref = escapes_find(ctx->env, varname);
    if (ref) {
      mem_ids = ref->expr->ea_md;
    }

    break;
  }

  default: {
    break;
  }
  }
  ast->ea_md = mem_ids;
  return mem_ids;
}
void escape_analysis(Ast *prog, EACtx *ctx) { ea(prog, ctx); }
