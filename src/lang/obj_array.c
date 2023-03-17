#include "obj_array.h"
#include "lang_memory.h"
#include "obj.h"

ObjArray *make_array(uint32_t size) {
  ObjArray *array = (ObjArray *)allocate_object(sizeof(ObjArray), OBJ_ARRAY);
  array->size = size;
  array->values = ALLOCATE(Value, size);
  return array;
}

ObjBuffer *make_buffer(uint32_t size, size_t data_size) {
  ObjBuffer *buffer =
      (ObjBuffer *)allocate_object(sizeof(ObjBuffer), OBJ_BUFFER);
  buffer->size = size;
  buffer->data_size = data_size;
  if (data_size == sizeof(int)) {
    buffer->values.ints = malloc(data_size * size);
  } else {
    buffer->values.doubles = malloc(data_size * size);
  }
  return buffer;
}
