#ifndef YLC_TYPE_HASH_H
#define YLC_TYPE_HASH_H

#include "type.h"
#include <stdint.h>

typedef enum {
  TYPE_HASH_OK,
  TYPE_HASH_INVALID,
  TYPE_HASH_NO_MEMORY,
  TYPE_HASH_TOO_DEEP,
} TypeHashStatus;

typedef struct {
  char *encoding;
  uint64_t hash;
} TypeHashKey;

// Builds an owned structural key; compare encodings after matching hashes.
// Aliases and inference metadata are excluded. Constructor/field names, variable
// IDs, function attributes, coroutine flags and closure layouts are included.
// Modules and unbound recursive references are unsupported. No types are cached
// or mutated; rebuild keys after mutation. This is not an RC equivalence test.
// Output must not own a previous encoding. Failure leaves it empty.
TypeHashStatus type_hash_create(const Type *type, TypeHashKey *key);
void type_hash_destroy(TypeHashKey *key);

#endif
