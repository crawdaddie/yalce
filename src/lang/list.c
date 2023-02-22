#include "list.h"
#include "memory.h"
#include "obj.h"
#include "util.h"
#include <string.h>
static ObjList *_make_list() {
  Value *values = calloc(sizeof(Value), 8);
  ObjList *list = malloc(sizeof(ObjList));
  list->object = (Object){OBJ_LIST};
  list->length = 0;
  list->_max = 8;
  list->values = values;
  return list;
}

Value make_list() {
  ObjList *obj_list = _make_list();
  return (Value){VAL_OBJ, {.object = (Object *)obj_list}};
}

static void append(ObjList *list, Value value) {
  if (list->length == list->_max) {
    int max = list->_max * 2;
    list->values = realloc(list->values, max);
    list->_max = max;
  }
  list->values[list->length] = value;
  list->length++;
}

void list_push(Value list, Value val) { append((ObjList *)AS_OBJ(list), val); }
