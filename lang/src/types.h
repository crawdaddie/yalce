#ifndef _LANG_TYPES_H
#define _LANG_TYPES_H
#include "ht.h"
#include "value.h"
typedef struct {
  const char *id;
  Value *type;
} type_map;

void add_type_lookups(ht *stack);
#endif
