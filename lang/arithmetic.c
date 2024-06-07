#include "arithmetic.h"
#include <math.h>

#define NUMERIC_OPERATION(op, l, r)                                            \
  if ((l).type == VALUE_INT && (r).type == VALUE_INT) {                        \
    return (Value){VALUE_INT, {.vint = (l).value.vint op(r).value.vint}};      \
  } else if ((l).type == VALUE_INT && (r).type == VALUE_NUMBER) {              \
    return (Value){VALUE_NUMBER, {.vnum = (l).value.vint op(r).value.vnum}};   \
  } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_INT) {              \
    return (Value){VALUE_NUMBER, {.vnum = (l).value.vnum op(r).value.vint}};   \
  } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_NUMBER) {           \
    return (Value){VALUE_NUMBER, {.vnum = (l).value.vnum op(r).value.vnum}};   \
  }

#define NUMERIC_COMPARISON_OPERATION(op, l, r)                                 \
  if ((l).type == VALUE_INT && (r).type == VALUE_INT) {                        \
    return (Value){VALUE_BOOL, {.vbool = (l).value.vint op(r).value.vint}};    \
  } else if ((l).type == VALUE_INT && (r).type == VALUE_NUMBER) {              \
    return (Value){VALUE_BOOL, {.vbool = (l).value.vint op(r).value.vnum}};    \
  } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_INT) {              \
    return (Value){VALUE_BOOL, {.vbool = (l).value.vnum op(r).value.vint}};    \
  } else if ((l).type == VALUE_NUMBER && (r).type == VALUE_NUMBER) {           \
    return (Value){VALUE_BOOL, {.vbool = (l).value.vnum op(r).value.vnum}};    \
  }

Value synth_add(Value l, Value r);

Value add_ops(Value l, Value r) {
  if (l.type == VALUE_SYNTH_NODE || r.type == VALUE_SYNTH_NODE) {
    return synth_add(l, r);
  }
  NUMERIC_OPERATION(+, l, r)
}

Value sub_ops(Value l, Value r) { NUMERIC_OPERATION(-, l, r); }

Value synth_mul(Value l, Value r);
Value mul_ops(Value l, Value r) {
  if (l.type == VALUE_SYNTH_NODE || r.type == VALUE_SYNTH_NODE) {
    return synth_mul(l, r);
  }

  NUMERIC_OPERATION(*, l, r);
}

Value div_ops(Value l, Value r) { NUMERIC_OPERATION(/, l, r); }

Value modulo_ops(Value l, Value r) {
  if (l.type == VALUE_INT && r.type == VALUE_INT) {
    return (Value){VALUE_INT, {.vint = l.value.vint % r.value.vint}};
  } else if (l.type == VALUE_INT && r.type == VALUE_NUMBER) {
    return (Value){VALUE_NUMBER, {.vnum = fmod(l.value.vint, r.value.vnum)}};
  } else if (l.type == VALUE_NUMBER && r.type == VALUE_INT) {
    return (Value){VALUE_NUMBER, {.vnum = fmod(l.value.vnum, r.value.vint)}};
  } else if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER) {
    return (Value){VALUE_NUMBER, {.vnum = fmod(l.value.vnum, r.value.vnum)}};
  }
}

Value lt_ops(Value l, Value r){NUMERIC_COMPARISON_OPERATION(<, l, r)}

Value lte_ops(Value l, Value r) {
  NUMERIC_COMPARISON_OPERATION(<=, l, r);
}
Value gt_ops(Value l, Value r) { NUMERIC_COMPARISON_OPERATION(>, l, r); }
Value gte_ops(Value l, Value r) { NUMERIC_COMPARISON_OPERATION(>=, l, r); }

inline static bool is_numeric(Value l) {
  return l.type == VALUE_INT || l.type == VALUE_NUMBER;
}

Value eq_ops(Value l, Value r) {
  Value res;
  if (is_numeric(l) && is_numeric(r)) {
    NUMERIC_COMPARISON_OPERATION(==, l, r);
  } else if (l.type == VALUE_STRING && r.type == VALUE_STRING) {
    res.type = VALUE_BOOL;
    res.value.vbool = (l.value.vstr.length == r.value.vstr.length) &&
                      (l.value.vstr.hash == r.value.vstr.hash);
    return res;
  } else if (l.type == VALUE_BOOL && r.type == VALUE_BOOL) {
    res.type = VALUE_BOOL;
    res.value.vbool = l.value.vbool == r.value.vbool;
    return res;
  }
  return res;
}

Value neq_ops(Value l, Value r) {
  Value res;
  if (is_numeric(l) && is_numeric(r)) {
    NUMERIC_COMPARISON_OPERATION(!=, l, r);
  } else if (l.type == VALUE_STRING && r.type == VALUE_STRING) {
    res.type = VALUE_BOOL;
    res.value.vbool = (l.value.vstr.length != r.value.vstr.length) ||
                      (l.value.vstr.hash != r.value.vstr.hash);
    return res;
  }
  return res;
}
