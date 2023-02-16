#ifndef _LANG_LIST_H
#define _LANG_LIST_H
#include "value.h"
Value make_list();
void list_push(Value *list, Value obj);
#endif
