#ifndef _LANG_TYPE_TYPECLASS_H
#define _LANG_TYPE_TYPECLASS_H
#include "parse.h"
#include <stdbool.h>
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

// class Arithmetic a where
//   (+) :: a -> a -> a
//   (-) :: a -> a -> a
//   (*) :: a -> a -> a
//   (/) :: a -> a -> a
//   (%) :: a -> a -> a
//
// instance Arithmetic Int where
//   (+)    = addInt
//   (-)    = subInt
//   (*)    = mulInt
//   (/)    = divInt
//   (%)    = remInt or modulo Int
//
// instance Arithmetic Float where
//   (+)    = addFloat
//   (*)    = mulFloat
//   etc ...
//
//
// a type a belongs to class Arithmetic if there are functions named (+), (-),
// (*), (/) & (%) of the appropriate types, defined on it.
// therefore the type expression Arithmetic a means
// ∃ functions (+), (-), (*), (/) & (%) for a
//
// square       :: Arithmetic a => a -> a
// square x     = x * x
// squares      :: Arithmetic a * Arithmetic b * Arithmetic c => (a,b,c) ->
// (a,b,c) squares (x, y, z) = (square x, square y, square z)

extern TypeClass TCArithmetic_int;
extern TypeClass TCOrd_int;
extern TypeClass TCEq_int;
extern TypeClass TCEq_bool;

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

bool implements(Type *t, TypeClass *tc);

TypeClass *derive_eq_for_type(Type *t);
TypeClass *derive_arithmetic_for_type(Type *t);
TypeClass *derive_ord_for_type(Type *t);

int add_typeclass(Type *t, TypeClass *tc);

Type *resolve_binop_typeclass(Type *l, Type *r, token_type op);

Type *resolve_op_typeclass_in_type(Type *l, token_type op);

TypeClass *find_op_typeclass_in_type(Type *t, token_type op, int *index);
#endif
