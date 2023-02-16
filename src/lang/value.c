#include "value.h"
#include "memory.h"

Value make_string(char *string) {
  Object *object = malloc(sizeof(Object));
  object->value = string;
  return OBJ_VAL(object);
}
void init_value_array(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}
void write_value_array(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values =
        GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}
void free_value_array(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}
