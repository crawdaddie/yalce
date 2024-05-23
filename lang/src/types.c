#include "types.h"
#include "value.h"

static native_symbol_map builtin_types[6] = {
    TYPE_MAP("int", VALUE_INT),    TYPE_MAP("double", VALUE_NUMBER),
    TYPE_MAP("str", VALUE_STRING), TYPE_MAP("bool", VALUE_BOOL),
    TYPE_MAP("void", VALUE_VOID),  TYPE_MAP("ptr", VALUE_OBJ)};

void add_type_lookups(ht *stack) {
  for (int i = 0; i < 6; i++) {
    native_symbol_map t = builtin_types[i];
    ht_set(stack, t.id, t.type);
  }
}

void register_type(ht *stack, native_symbol_map tmap) {
  ht_set(stack, tmap.id, tmap.type);
}
