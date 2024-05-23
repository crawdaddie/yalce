#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define NUMERIC_OPERATION(op, l, r, res)                                       \
  do {                                                                         \
    if ((l).type == VALUE_INT && (r).type == VALUE_INT) {                      \
      (res)->type = VALUE_INT;                                                 \
      (res)->value.vint = (l).value.vint op(r).value.vint;                     \
    } else if ((l).type == VALUE_INT && (r).type == VALUE_NUMBER) {            \
      (res)->type = VALUE_NUMBER;                                              \
      (res)->value.vnum = (l).value.vint op(r).value.vnum;                     \
    } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_INT) {            \
      (res)->type = VALUE_NUMBER;                                              \
      (res)->value.vnum = (l).value.vnum op(r).value.vint;                     \
    } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_NUMBER) {         \
      (res)->type = VALUE_NUMBER;                                              \
      (res)->value.vnum = (l).value.vnum op(r).value.vnum;                     \
    }                                                                          \
  } while (0)

#define NUMERIC_COMPARISON_OPERATION(op, l, r, res)                            \
  do {                                                                         \
    res->type = VALUE_BOOL;                                                    \
    if ((l).type == VALUE_INT && (r).type == VALUE_INT) {                      \
      (res)->value.vbool = (l).value.vint op(r).value.vint;                    \
    } else if ((l).type == VALUE_INT && (r).type == VALUE_NUMBER) {            \
      (res)->value.vbool = (l).value.vint op(r).value.vnum;                    \
    } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_INT) {            \
      (res)->value.vbool = (l).value.vnum op(r).value.vint;                    \
    } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_NUMBER) {         \
      (res)->value.vbool = (l).value.vnum op(r).value.vnum;                    \
    }                                                                          \
  } while (0)

Value add_ops(Value l, Value r, Value *res) {
  NUMERIC_OPERATION(+, l, r, res);
  return *res;
}
Value sub_ops(Value l, Value r, Value *res) {
  NUMERIC_OPERATION(-, l, r, res);
  return *res;
}
Value mul_ops(Value l, Value r, Value *res) {
  NUMERIC_OPERATION(*, l, r, res);
  return *res;
}

Value div_ops(Value l, Value r, Value *res) {
  NUMERIC_OPERATION(/, l, r, res);
  return *res;
}

Value modulo_ops(Value l, Value r, Value *res) {
  if (l.type == VALUE_INT && r.type == VALUE_INT) {
    res->type = VALUE_INT;
    res->value.vint = l.value.vint % r.value.vint;
    return *res;
  } else if (l.type == VALUE_INT && r.type == VALUE_NUMBER) {
    res->type = VALUE_NUMBER;
    res->value.vnum = fmod(l.value.vint, r.value.vnum);
    return *res;
  } else if (l.type == VALUE_NUMBER && r.type == VALUE_INT) {
    res->type = VALUE_NUMBER;
    res->value.vnum = fmod(l.value.vnum, r.value.vint);
    return *res;
  } else if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER) {
    res->type = VALUE_NUMBER;
    res->value.vnum = fmod(l.value.vnum, r.value.vnum);
    return *res;
  }
  return *res;
}

Value lt_ops(Value l, Value r, Value *res) {
  NUMERIC_COMPARISON_OPERATION(<, l, r, res);
  return *res;
}

Value lte_ops(Value l, Value r, Value *res) {
  NUMERIC_COMPARISON_OPERATION(<=, l, r, res);
  return *res;
}
Value gt_ops(Value l, Value r, Value *res) {
  NUMERIC_COMPARISON_OPERATION(>, l, r, res);
  return *res;
}
Value gte_ops(Value l, Value r, Value *res) {
  NUMERIC_COMPARISON_OPERATION(>=, l, r, res);
  return *res;
}
inline static bool is_numeric(Value l) {
  return l.type == VALUE_INT || l.type == VALUE_NUMBER;
}

Value eq_ops(Value l, Value r, Value *res) {
  if (is_numeric(l) && is_numeric(r)) {
    NUMERIC_COMPARISON_OPERATION(==, l, r, res);
  } else if (l.type == VALUE_STRING && r.type == VALUE_STRING) {
    res->type = VALUE_BOOL;
    res->value.vbool = (l.value.vstr.length == r.value.vstr.length) &&
                       (l.value.vstr.hash == r.value.vstr.hash);
    return *res;
  }
  return *res;
}

Value neq_ops(Value l, Value r, Value *res) {
  if (is_numeric(l) && is_numeric(r)) {
    NUMERIC_COMPARISON_OPERATION(!=, l, r, res);
  } else if (l.type == VALUE_STRING && r.type == VALUE_STRING) {
    res->type = VALUE_BOOL;
    res->value.vbool = (l.value.vstr.length != r.value.vstr.length) ||
                       (l.value.vstr.hash != r.value.vstr.hash);
    return *res;
  }
  return *res;
}
