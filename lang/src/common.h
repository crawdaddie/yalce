#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include <stdint.h>

typedef struct {
  char *chars;
  int length;
  uint64_t hash;
} ObjString;

uint64_t hash_string(const char *key, int length);
uint64_t hash_key(const char *key);

#endif
