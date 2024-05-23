#ifndef _LANG_NATIVE_FUNCTIONS_H
#define _LANG_NATIVE_FUNCTIONS_H
#include "ht.h"
#include "value.h"

void add_native_functions(ht *stack);

int _list_length(int argc, Value *argv);

Value list_nth(int n, Value *list);
#endif
