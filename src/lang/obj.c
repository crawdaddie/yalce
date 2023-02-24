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
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = strlen(chars);
  string->hash = hash_string(chars, string->length);
  string->chars = chars;
  return string;
}

ObjString *make_string(char *chars) {
  ObjString *string = _make_string(chars);
  return string;
}
