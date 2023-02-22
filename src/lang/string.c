#include "string.h"
#include <string.h>
static ObjString *_make_string(char *string) {
  ObjString *string_obj = malloc(sizeof(ObjString));
  string_obj->object = (Object){OBJ_STRING};
  string_obj->length = strlen(string);
  string_obj->chars = string;
  return string_obj;
}

Value make_string(char *string) {
  ObjString *obj = _make_string(string);
  return (Value){VAL_OBJ, {.object = (Object *)obj}};
}
