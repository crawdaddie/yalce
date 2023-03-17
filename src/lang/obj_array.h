#ifndef _LANG_ARRAY_H
#define _LANG_ARRAY_H
#include "chunk.h"
#include "obj.h"

typedef struct {
  Object object;
  int size;
  Value *values;
} ObjArray;

ObjArray *make_array(uint32_t size);
#endif
