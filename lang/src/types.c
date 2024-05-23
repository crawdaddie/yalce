#include "types.h"

static type_map builtin_types[6] = {
    {.id = "int", &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_INT}}},
    {.id = "double",
     &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_NUMBER}}},
    {.id = "str",
     &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_STRING}}},
    {.id = "bool", &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_BOOL}}},
    {.id = "void", &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_VOID}}},
    {.id = "ptr", &(Value){.type = VALUE_TYPE, .value = {.type = VALUE_OBJ}}},
};

void add_type_lookups(ht *stack) {
  for (int i = 0; i < 6; i++) {
    type_map t = builtin_types[i];
    ht_set(stack, t.id, t.type);
  }
}

void register_type(ht *stack, type_map tmap) {
  ht_set(stack, tmap.id, tmap.type);
}
