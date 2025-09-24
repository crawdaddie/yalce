#include "./typeclass_resolve.h"
#include "types/common.h"
#include "types/inference.h"
#include "types/type.h"
#include <string.h>

double tc_rank(const char *tc_name, Type *t) {
  TypeClass *tc = t->implements;
  for (TypeClass *tc = t->implements; tc; tc = tc->next) {
    if (CHARS_EQ(tc->name, tc_name)) {
      return tc->rank;
    }
  }
  return 0.;
}
Type *tc_resolve(Type *tcr) {
  const char *tc_name = tcr->data.T_CONS.name;
  Type *a = tcr->data.T_CONS.args[0];
  Type *b = tcr->data.T_CONS.args[1];

  if (b->kind == T_TYPECLASS_RESOLVE) {
    b = tc_resolve(b);
  }

  if (tc_rank(tc_name, a) >= tc_rank(tc_name, b)) {
    return a;
  }
  return b;
}

bool type_list_contains(TypeList *l, Type *t) {
  for (TypeList *c = l; c; c = c->next) {
    if (types_equal(c->type, t)) {
      return true;
    }
  }
  return false;
}

Type *construct_tc_resolve(const char *tc_name, TypeList *types) {
  if (types->next == NULL) {
    return types->type;
  }

  Type *tc = empty_type();
  tc->kind = T_TYPECLASS_RESOLVE;
  tc->data.T_CONS.name = tc_name;
  tc->data.T_CONS.args = t_alloc(sizeof(Type *) * 2);
  tc->data.T_CONS.num_args = 2;
  tc->data.T_CONS.args[0] = types->type;
  tc->data.T_CONS.args[1] = construct_tc_resolve(tc_name, types->next);
  return tc;
}

Type *_cleanup_tc_resolve(const char *tc_name, TypeList *types, Type *t) {
  if (t->kind != T_TYPECLASS_RESOLVE) {
    if (!type_list_contains(types, t)) {
      TypeList node = {.type = t, .next = types}; // Stack allocation
      types = &node;
    }
    Type *clean = construct_tc_resolve(tc_name, types);
    return clean;
  }

  Type *a = t->data.T_CONS.args[0];
  Type *b = t->data.T_CONS.args[1];

  if (!type_list_contains(types, a)) {
    TypeList node = {.type = a, .next = types}; // Stack allocation
    return _cleanup_tc_resolve(tc_name, &node, b);
  }

  return _cleanup_tc_resolve(tc_name, types, b);
}

Type *cleanup_tc_resolve(Type *t) {
  Type *clean = _cleanup_tc_resolve(t->data.T_CONS.name, NULL, t);
  clean->implements = t->implements;
  return clean;
}
