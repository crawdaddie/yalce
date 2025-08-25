#include "./type_expressions.h"
#include "serde.h"
Scheme *compute_type_expression(Ast *expr, TICtx *ctx) {
  switch (expr->tag) {
  case AST_IDENTIFIER: {

    Scheme *ts = lookup_scheme(ctx->env, expr->data.AST_IDENTIFIER.value);
    return ts;
  }

  case AST_TUPLE: {
    int len = expr->data.AST_LIST.len;
    Type **members = talloc(sizeof(Type *) * len);

    VarList *vars = NULL;
    for (int i = 0; i < len; i++) {
      Scheme *mem = compute_type_expression(expr->data.AST_LIST.items + i, ctx);

      members[i] = mem->type;

      for (VarList *mv = mem->vars; mv; mv = mv->next) {
        varlist_add(vars, mv->var);
      }
    }

    Type *tuple_type = create_tuple_type(len, members);

    Scheme *sch = talloc(sizeof(Scheme));
    *sch = (Scheme){.vars = vars, .type = tuple_type};
    return sch;
  }
  default: {
    return NULL;
  }
  }
}
