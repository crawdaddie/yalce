#ifndef _LANG_LIST_H
#define _LANG_LIST_H
#include "value.h"

typedef struct {
  Object object;
  int length;
  Value *values;
  int _max;
} ObjList;

Value make_list();
void list_push(Value list, Value element);
#endif
