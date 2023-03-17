#include "obj_array.h"
#include "lang_memory.h"
#include "obj.h"

ObjArray *make_array(uint32_t size) {
  ObjArray *array = (ObjArray *)allocate_object(sizeof(ObjArray), OBJ_ARRAY);
  array->size = size;
  array->values = ALLOCATE(Value, size);
  return array;
}
