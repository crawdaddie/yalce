#include "obj_array.h"
#include "lang_memory.h"
#include "obj.h"
#include "common.h"

ObjArray *make_array(uint32_t size) {
  ObjArray *array = (ObjArray *)allocate_object(sizeof(ObjArray), OBJ_ARRAY);
  array->size = size;
  array->values = ALLOCATE(Value, size);
  return array;
}

ObjBuffer *make_buffer(uint32_t size, size_t data_size) {
  ObjBuffer *buffer; 
  switch (data_size) {
    case BUF_INT: {
      buffer =
          (ObjBuffer *)allocate_object(sizeof(ObjBufInt), OBJ_BUFFER);
      ((ObjBufInt *)buffer)->values = malloc(data_size * size);
      buffer->data_size = data_size;
      buffer->size = size;
      break;
    }

    case BUF_DOUBLE: {
      buffer =
          (ObjBuffer *)allocate_object(sizeof(ObjBufInt), OBJ_BUFFER);
      ((ObjBufDouble *)buffer)->values = malloc(data_size * size);
      buffer->data_size = data_size;
      buffer->size = size;
      break;
    }

  }
  return (ObjBuffer *)buffer;
}

static void buffer_set(ObjBuffer *buf, int index, Value value) {
  if (buf->data_size == BUF_INT) {
    ((ObjBufInt *)buf)->values[index] = AS_INTEGER(value);
    return;
  } 
  ((ObjBufDouble *)buf)->values[index] = AS_NUMBER(value);
}


void array_set(Object *array, int index, Value value) {
  if (array->type == OBJ_BUFFER) {
    return buffer_set((ObjBuffer *)array, index, value);
  }
  ((ObjArray *)array)->values[index] = value;

}
