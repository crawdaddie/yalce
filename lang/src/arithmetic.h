#ifndef _LANG_ARITHMETIC_H
#define _LANG_ARITHMETIC_H
#include "value.h"

Value add_ops(Value l, Value r, Value *res);
Value sub_ops(Value l, Value r, Value *res);
Value mul_ops(Value l, Value r, Value *res);
Value div_ops(Value l, Value r, Value *res);
Value modulo_ops(Value l, Value r, Value *res);
Value lt_ops(Value l, Value r, Value *res);
Value lte_ops(Value l, Value r, Value *res);
Value gt_ops(Value l, Value r, Value *res);
Value gte_ops(Value l, Value r, Value *res);
Value eq_ops(Value l, Value r, Value *res);
Value neq_ops(Value l, Value r, Value *res);
#endif
