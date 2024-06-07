#include "common.h"
// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
//
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
uint64_t hash_string(const char *key, int length) {
  uint64_t hash = FNV_OFFSET;
  for (int i = 0; i < length; i++) {
    const char *p = key + i;
    hash ^= (uint64_t)(unsigned char)(*p);
    hash *= FNV_PRIME;
  }
  return hash;
}

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t hash_key(const char *key) {
  uint64_t hash = FNV_OFFSET;
  for (const char *p = key; *p; p++) {
    hash ^= (uint64_t)(unsigned char)(*p);
    hash *= FNV_PRIME;
  }
  return hash;
}
