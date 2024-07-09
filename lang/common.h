#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include "ht.h"
#include <stdint.h>

typedef struct {
  const char *chars;
  int length;
  uint64_t hash;
} ObjString;

uint64_t hash_string(const char *key, int length);
uint64_t hash_key(const char *key);
#include <stdio.h>

#endif
