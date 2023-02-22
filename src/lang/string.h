#ifndef _LANG_STRING_H
#define _LANG_STRING_H
#include "obj.h"
#include "value.h"

typedef struct {
  Object object;
  int length;
  char *chars;
} ObjString;

Value make_string(char *string);
#endif
