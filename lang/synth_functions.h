#ifndef _LANG_SYNTH_FUNCTIONS_H
#define _LANG_SYNTH_FUNCTIONS_H
#include "ht.h"
#include "value.h"

void add_synth_functions(ht *stack);
Value synth_add(Value l, Value r);
Value synth_mul(Value l, Value r);

#endif
