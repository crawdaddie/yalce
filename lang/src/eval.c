#include "eval.h"
#include <math.h>
#include <stdlib.h>

#define NUMERIC_OPERATION(op, l, r)                                            \
  do {                                                                         \
    if ((l)->type == VALUE_INT && (r)->type == VALUE_INT) {                    \
      (l)->value.vint = (l)->value.vint op(r)->value.vint;                     \
      return (l);                                                              \
    } else if ((l)->type == VALUE_INT && (r)->type == VALUE_NUMBER) {          \
      (l)->type = VALUE_NUMBER;                                                \
      (l)->value.vnum = (l)->value.vint op(r)->value.vnum;                     \
      return (l);                                                              \
    } else if ((l)->type == VALUE_NUMBER && (r)->type == VALUE_INT) {          \
      (l)->value.vnum = (l)->value.vnum op(r)->value.vint;                     \
      return (l);                                                              \
    } else if ((l)->type == VALUE_NUMBER && (r)->type == VALUE_NUMBER) {       \
      (l)->value.vnum = (l)->value.vnum op(r)->value.vnum;                     \
      return (l);                                                              \
    }                                                                          \
  } while (0)

#define NUMERIC_COMPARISON_OPERATION(op, l, r)                                 \
  do {                                                                         \
    if ((l)->type == VALUE_INT && (r)->type == VALUE_INT) {                    \
      (l)->type = VALUE_BOOL;                                                  \
      (l)->value.vbool = (l)->value.vint op(r)->value.vint;                    \
      return (l);                                                              \
    } else if ((l)->type == VALUE_INT && (r)->type == VALUE_NUMBER) {          \
      (l)->type = VALUE_BOOL;                                                  \
      (l)->value.vbool = (l)->value.vint op(r)->value.vnum;                    \
      return (l);                                                              \
    } else if ((l)->type == VALUE_NUMBER && (r)->type == VALUE_INT) {          \
      (l)->type = VALUE_BOOL;                                                  \
      (l)->value.vbool = (l)->value.vnum op(r)->value.vint;                    \
      return (l);                                                              \
    } else if ((l)->type == VALUE_NUMBER && (r)->type == VALUE_NUMBER) {       \
      (l)->type = VALUE_BOOL;                                                  \
      (l)->value.vbool = (l)->value.vnum op(r)->value.vnum;                    \
      return (l);                                                              \
    }                                                                          \
  } while (0)

static Value *add_ops(Value *l, Value *r) {
  NUMERIC_OPERATION(+, l, r);
  return NULL;
}

static Value *sub_ops(Value *l, Value *r) {
  NUMERIC_OPERATION(-, l, r);
  return NULL;
}

static Value *mul_ops(Value *l, Value *r) {
  NUMERIC_OPERATION(*, l, r);
  return NULL;
}

static Value *div_ops(Value *l, Value *r) {
  NUMERIC_OPERATION(-, l, r);
  return NULL;
}
static Value *modulo_ops(Value *l, Value *r) {
  if (l->type == VALUE_INT && r->type == VALUE_INT) {
    l->value.vint = l->value.vint % r->value.vint;
    return l;
  } else if (l->type == VALUE_INT && r->type == VALUE_NUMBER) {
    l->type = VALUE_NUMBER;
    l->value.vnum = fmod(l->value.vint, r->value.vnum);
    return l;
  } else if (l->type == VALUE_NUMBER && r->type == VALUE_INT) {
    l->value.vnum = fmod(l->value.vnum, r->value.vint);
    return l;
  } else if (l->type == VALUE_NUMBER && r->type == VALUE_NUMBER) {
    l->value.vnum = fmod(l->value.vnum, r->value.vnum);
    return l;
  }
  return NULL;
}

static Value *lt_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(<, l, r);
  return NULL;
}

static Value *lte_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(<=, l, r);
  return NULL;
}
static Value *gt_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(>, l, r);
  return NULL;
}
static Value *gte_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(>=, l, r);
  return NULL;
}
static Value *eq_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(==, l, r);
  return NULL;
}
static Value *neq_ops(Value *l, Value *r) {
  NUMERIC_COMPARISON_OPERATION(!=, l, r);
  return NULL;
}

Value *eval(Ast *ast, Value *val) {
  if (!ast) {
    return NULL;
  }

  switch (ast->tag) {

  case AST_BODY: {
    void *final;
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      Value *val = malloc(sizeof(Value));
      final = eval(stmt, val);
    }
    return final;
  }

  case AST_NUMBER: {
    val->type = VALUE_NUMBER;
    val->value.vnum = ast->data.AST_NUMBER.value;
    return val;
  }

  case AST_INT: {
    val->type = VALUE_INT;
    val->value.vint = ast->data.AST_INT.value;
    return val;
  }

  case AST_STRING: {
    val->type = VALUE_STRING;
    val->value.vstr = ast->data.AST_STRING.value;
    return val;
  }

  case AST_BOOL: {
    val->type = VALUE_BOOL;
    val->value.vbool = ast->data.AST_BOOL.value;
    return val;
  }
  case AST_BINOP: {
    Value *l = eval(ast->data.AST_BINOP.left, val);
    Value *r = malloc(sizeof(Value));
    r = eval(ast->data.AST_BINOP.right, r);
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS: {
      l = add_ops(l, r);
      break;
    }
    case TOKEN_MINUS: {
      l = sub_ops(l, r);
      break;
    }
    case TOKEN_STAR: {
      l = mul_ops(l, r);
      break;
    }
    case TOKEN_SLASH: {
      l = div_ops(l, r);
      break;
    }
    case TOKEN_MODULO: {
      l = modulo_ops(l, r);
      break;
    }
    case TOKEN_LT: {
      l = lt_ops(l, r);
      break;
    }
    case TOKEN_LTE: {
      l = lte_ops(l, r);
      break;
    }
    case TOKEN_GT: {
      l = gt_ops(l, r);
      break;
    }
    case TOKEN_GTE: {
      l = gte_ops(l, r);
      break;
    }
    case TOKEN_EQUALITY: {
      l = eq_ops(l, r);
      break;
    }
    case TOKEN_NOT_EQUAL: {
      l = neq_ops(l, r);
      break;
    }
    }
    free(r);
    return l;
  }
  }
}

void print_value(Value *val) {
  if (!val) {
    return;
  }

  switch (val->type) {
  case VALUE_INT:
    printf("[%d]", val->value.vint);
    break;

  case VALUE_NUMBER:
    printf("[%f]", val->value.vnum);
    break;

  case VALUE_STRING:
    printf("[%s]", val->value.vstr);
    break;

  case VALUE_BOOL:
    printf("[%s]", val->value.vbool ? "true" : "false");
    break;

  case VALUE_VOID:
    printf("[()]");
    break;
  }
}
