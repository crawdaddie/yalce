#include "native_functions.h"
#include "value.h"
#include <stdlib.h>

Value _strlen(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

Value type_of(int argc, Value *argv) { return INT(argv->type); }

int _list_length(int argc, Value *argv) {
  IntList *list = (IntList *)(*argv).value.vlist;
  return list->len;
}

Value list_length(int argc, Value *argv) { return INT(_list_length(1, argv)); }

value_type _list_type(Value *argv) {
  IntList *list = (IntList *)(*argv).value.vlist;
  return list->type;
}
Value list_type(int argc, Value *argv) { return INT(_list_type(argv)); }

#define LIST_NTH(list, list_type, item_type, n)                                \
  item_type item = ((list_type *)list->value.vlist)->items[n]

Value list_nth(int n, Value *list) {

  switch (_list_type(list)) {
  case VALUE_INT: {
    LIST_NTH(list, IntList, int, n);
    return INT(item);
  }
  case VALUE_NUMBER: {
    LIST_NTH(list, NumberList, double, n);
    return NUM(item);
  }
  case VALUE_STRING: {
    ObjString str = {
        .chars = ((StringList *)list->value.vlist)->items[n],
        .length = ((StringList *)list->value.vlist)->lens[n],
        .hash = ((StringList *)list->value.vlist)->hashes[n],

    };
    return STRING(str);
  }
  case VALUE_BOOL: {
    LIST_NTH(list, IntList, bool, n);
    return BOOL(item);
  }
  case VALUE_OBJ: {
    LIST_NTH(list, ObjList, void *, n);
    return OBJ(item);
  }
  case VALUE_LIST: {
    LIST_NTH(list, ObjList, void *, n);
    return LIST(item);
  }
  }
}

Value _strconcat(int argc, Value *argv) {
  Value string = *argv;
  return INT(string.value.vstr.length);
}

Value _print(int argc, Value *argv) {
  printf("%s", argv->value.vstr.chars);
  return (Value){VALUE_VOID};
}
#define EXTEND_LIST(argv, list_type, item_type, val)                           \
  item_type *items = ((list_type *)argv->value.vlist)->items;                  \
  ((list_type *)argv->value.vlist)->len++;                                     \
  int len = ((list_type *)argv->value.vlist)->len;                             \
  ((list_type *)argv->value.vlist)->items =                                    \
      realloc(items, sizeof(item_type) * len);                                 \
  ((list_type *)argv->value.vlist)->items[len - 1] = val

Value list_push(int argc, Value *argv) {
  Value *list = argv + 1;
  Value *val = argv;

  switch (_list_type(list)) {
  case VALUE_INT: {
    EXTEND_LIST(list, IntList, int, (val)->value.vint);
    break;
  }
  case VALUE_NUMBER: {
    EXTEND_LIST(list, NumberList, double, (val)->value.vnum);
    break;
  }
  case VALUE_STRING: {
    ObjString str = (val)->value.vstr;

    EXTEND_LIST(list, StringList, char *, str.chars);
    int ll = ((StringList *)list->value.vlist)->len;

    ((StringList *)list->value.vlist)->lens =
        realloc(((StringList *)list->value.vlist)->lens, sizeof(int) * ll);
    ((StringList *)list->value.vlist)->lens[ll - 1] = str.length;

    ((StringList *)list->value.vlist)->hashes = realloc(
        ((StringList *)list->value.vlist)->hashes, sizeof(uint64_t) * ll);
    ((StringList *)list->value.vlist)->hashes[ll - 1] = str.hash;

    break;
  }
  case VALUE_BOOL: {
    EXTEND_LIST(list, IntList, bool, (val)->value.vbool);
    break;
  }
  case VALUE_OBJ: {
    EXTEND_LIST(list, ObjList, void *, (val)->value.vobj);
    break;
  }
  case VALUE_LIST: {
    EXTEND_LIST(list, ObjList, void *, (val)->value.vlist);
    break;
  }
  }
  return *list;
}

#define NUM_NATIVES 6
static native_symbol_map builtin_native_fns[NUM_NATIVES] = {
    {"strlen", NATIVE_FN(_strlen, 1)},
    {"print", NATIVE_FN(_print, 1)},
    {"List.length", NATIVE_FN(list_length, 1)},
    {"List.type", NATIVE_FN(list_type, 1)},
    {"List.push", NATIVE_FN(list_push, 2)},
    {"type_of", NATIVE_FN(type_of, 1)},
};

void add_native_functions(ht *stack) {
  for (int i = 0; i < NUM_NATIVES; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }
}
