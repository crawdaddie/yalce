#ifndef _LANG_ARRAY_H
#define _LANG_ARRAY_H
#include "chunk.h"
#include "obj.h"

typedef struct {
  Object object;
  int size;
  Value *values;
} ObjArray;

typedef struct {
  Object object;
  int size;
  int data_size;
  union {
    int *ints;
    double *doubles;
  } values;
} ObjBuffer;

ObjArray *make_array(uint32_t size);
ObjBuffer *make_buffer(uint32_t size, size_t data_size);
#define ARRAY_VALUES(value) ((ObjArray *)AS_OBJ(value))->values;

#endif
