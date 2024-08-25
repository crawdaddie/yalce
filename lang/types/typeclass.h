#ifndef _LANG_TYPE_TYPECLASS_H
#define _LANG_TYPE_TYPECLASS_H

#include "common.h"

// forward declare Type
struct Type;
typedef struct TypeClass {
  const char *name;
  struct Type *method_signature;
  double rank; // used to resolve implicit type conversions
  // eg 1.0 (double) + 2 (int) = double
} TypeClass;

extern TypeClass int_tc_table[];
extern TypeClass uint64_tc_table[];
extern TypeClass num_tc_table[];

TypeClass *find_op_impl(struct Type t, token_type op);
#endif
