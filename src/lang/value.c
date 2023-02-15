#include "value.h"
Value make_string(char *string) {
  Object *object = malloc(sizeof(Object));
  object->value = string;
  return OBJ_VAL(object);
}
