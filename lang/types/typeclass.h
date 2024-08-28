#ifndef _LANG_TYPE_TYPECLASS_H
#define _LANG_TYPE_TYPECLASS_H
#include <stddef.h>

typedef struct Type Type;

typedef struct Method {
  const char *name;
  void *method;
  size_t size;
  Type *signature;
} Method;

typedef struct TypeClass {
  const char *name;
  size_t num_methods;
  double rank;
  Method *methods;
} TypeClass;

extern TypeClass TCArithmetic_int;
extern TypeClass TCOrd_int;
extern TypeClass TCEq_int;

extern TypeClass TCArithmetic_uint64;
extern TypeClass TCOrd_uint64;
extern TypeClass TCEq_uint64;

extern TypeClass TCArithmetic_num;
extern TypeClass TCOrd_num;
extern TypeClass TCEq_num;

int find_typeclass_for_method(Type *t, const char *method_name, TypeClass *tc,
                              Type *method_signature);

TypeClass *get_typeclass(Type *t, TypeClass *tc);
TypeClass *get_typeclass_by_name(Type *t, const char *name);

Type *typeclass_method_signature(TypeClass *tc, const char *name);

#endif
