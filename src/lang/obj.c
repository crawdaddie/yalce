#include "obj.h"
#include "memory.h"
#include "sym.h"
#include "vm.h"
#include <string.h>

#define ALLOCATE_OBJ(type, object_type)                                        \
  (type *)allocate_object(sizeof(type), object_type)

static Object *allocate_object(size_t size, ObjectType type) {
  Object *object = (Object *)reallocate(NULL, 0, size);
  object->type = type;
  /* object->isMarked = false; */

  /* object->next = vm.objects; */
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif

  return object;
}
static ObjString *_make_string(char *chars) {
  int len = strlen(chars);
  uint32_t hash = hash_string(chars, len);
  ObjString *interned = table_find_string(&vm.strings, chars, len, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, len + 1);
    return interned;
  }

  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = len;
  string->hash = hash;
  string->chars = chars;
  table_set(&vm.strings, string, NIL_VAL);
  return string;
}

ObjString *make_string(char *chars) {
  ObjString *string = _make_string(chars);
  return string;
}

ObjFunction *make_function() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  /* init_chunk(&function->chunk); */
  return function;
}
