#ifndef _LANG_ARITHMETIC_H
#define _LANG_ARITHMETIC_H
#include "value.h"

Value add_ops(Value l, Value r);
Value sub_ops(Value l, Value r);
Value mul_ops(Value l, Value r);
Value div_ops(Value l, Value r);
Value modulo_ops(Value l, Value r);
Value lt_ops(Value l, Value r);
Value lte_ops(Value l, Value r);
Value gt_ops(Value l, Value r);
Value gte_ops(Value l, Value r);
Value eq_ops(Value l, Value r);
Value neq_ops(Value l, Value r);
#endif
