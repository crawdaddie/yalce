#ifndef _LANG_ARRAY_H
#define _LANG_ARRAY_H
#include "chunk.h"
#include "obj.h"

typedef struct {
  Object object;
  int size;
  Value *values;
} ObjArray;

typedef union buf_values {
  int *ints;
  double *doubles;
} buf_values;

typedef struct {
  Object object;
  int size;
  int data_size;
} ObjBuffer;

typedef struct {
  ObjBuffer buftype;
  int *values;
} ObjBufInt;

typedef struct {
  ObjBuffer buftype;
  double *values;
} ObjBufDouble;

ObjArray *make_array(uint32_t size);
ObjBuffer *make_buffer(uint32_t size, size_t data_size);

void array_set(Object *array, int index, Value value);
Value array_get(Object *array, int index);

#define ARRAY_VALUES(value) ((ObjArray *)AS_OBJ(value))->values;
#define IS_BUF(value) AS_OBJ(value)->type == OBJ_BUFFER

#endif
