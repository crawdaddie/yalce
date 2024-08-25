#ifndef _LANG_TYPE_TYPECLASS_H
#define _LANG_TYPE_TYPECLASS_H

#include "common.h"

// forward declare Type
struct Type;

typedef struct TypeClassImpl {
  const char *name;
  struct Type *method_signature;
} TypeClassImpl;

typedef struct TypeClass {
  const char *name;
  int num;
  TypeClassImpl *methods;
  double rank; // used to resolve implicit type conversions
  // eg 1.0 (double) + 2 (int) = double
} TypeClass;

extern TypeClass int_traits[];
extern TypeClass uint64_traits[];
extern TypeClass num_traits[];

TypeClassImpl *find_op_impl(TypeClass t, token_type op);

void merge_typeclasses(struct Type *to, struct Type *from);
#endif
