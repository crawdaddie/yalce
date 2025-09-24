#include "type.h"
#include "serde.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *empty_type();

Type *env_lookup(TypeEnv *env, const char *name);
void reset_type_var_counter();
Type *create_option_type(Type *option_of);

bool types_equal(Type *t1, Type *t2) {
  if (t1 == t2) {
    return true;
  }

  if (t1 == NULL || t2 == NULL) {
    return false;
  }

  if (t1->kind != t2->kind) {
    return false;
  }

  switch (t1->kind) {
  case T_INT:
  case T_NUM:
  case T_STRING:
  case T_BOOL:
  case T_CHAR:
  case T_VOID:
  case T_EMPTY_LIST: {
    return true;
  }

  case T_VAR: {

    bool eq = strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0;
    if (t2->implements != NULL) {
    }
    return eq;
  }

  case T_TYPECLASS_RESOLVE:
  case T_CONS: {

    if (t1->alias && t2->alias && (strcmp(t1->alias, t2->alias) != 0)) {
      return false;
    }
    if (strcmp(t1->data.T_CONS.name, t2->data.T_CONS.name) != 0) {
      return false;

    } else if (t1->data.T_CONS.num_args != t2->data.T_CONS.num_args) {
      return false;
    }
    bool eq = true;
    for (int i = 0; i < t1->data.T_CONS.num_args; i++) {
      eq &= types_equal(t1->data.T_CONS.args[i], t2->data.T_CONS.args[i]);
    }

    return eq;
  }
  case T_FN: {
    if (types_equal(t1->data.T_FN.from, t2->data.T_FN.from)) {
      return types_equal(t1->data.T_FN.to, t2->data.T_FN.to) &&
             (t1->is_coroutine_instance == t2->is_coroutine_instance);
    }
    return false;
  }

  case T_SCHEME: {
    return types_equal(t1->data.T_SCHEME.type, t2->data.T_SCHEME.type);
  }
  }
  return false;
}

void tfree(void *mem) {
  // free(mem);
}

Type *tvar(const char *name) {
  Type *mem = empty_type();
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  mem->kind = T_VAR;
  mem->data.T_VAR = t_alloc(sizeof(char) * strlen(name));
  memcpy(mem->data.T_VAR, name, strlen(name));
  return mem;
}

bool is_generic(Type *t) {
  if (t == NULL) {
    fprintf(stderr, "Error type passed to generic test is null\n");
    return NULL;
  }

  switch (t->kind) {
  case T_VAR: {
    if (t->is_recursive_type_ref) {
      return false;
    }
    return true;
  }

  case T_TYPECLASS_RESOLVE:
  case T_CONS: {
    if (t->data.T_CONS.num_args == 0) {
      return false;
    }
    if (strcmp(t->data.T_CONS.name, TYPE_NAME_VARIANT) == 0) {
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        Type *arg = t->data.T_CONS.args[i];
        if (is_generic(arg)) {
          return true;
        }
      }
      return false;

    } else if (strcmp(t->data.T_CONS.name, "forall") == 0) {
      return true;
    } else {
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        if (is_generic(t->data.T_CONS.args[i])) {
          return true;
        }
      }
      return false;
    }
  }

  case T_FN: {
    return is_generic(t->data.T_FN.from) || is_generic(t->data.T_FN.to);
  }

  default:
    return false;
  }
}

Type *type_fn(Type *from, Type *to) {
  Type *fn = empty_type();
  fn->kind = T_FN;
  fn->data.T_FN.from = from;
  fn->data.T_FN.to = to;
  return fn;
}

Type *fn_return_type(Type *fn) {
  if (fn->kind != T_FN) {
    // If it's not a function type, it's the return type itself
    return fn;
  }
  // Recursively check the 'to' field
  return fn_return_type(fn->data.T_FN.to);
}

Type *create_type_multi_param_fn(int len, Type **from, Type *to) {
  Type *fn = to;
  for (int i = len - 1; i >= 0; i--) {
    Type *ptype = from[i];
    // print_type(ptype);
    fn = type_fn(ptype, fn);
  }
  return fn;
}

int fn_type_args_len(Type *fn_type) {

  if (fn_type->data.T_FN.from->kind == T_VOID) {
    return 1;
  }

  int fn_len = 0;

  for (Type *ct = fn_type; ct->kind == T_FN && !(is_closure(ct->data.T_FN.to));
       ct = ct->data.T_FN.to) {
    fn_len++;
  }

  return fn_len;
}

Type *create_tuple_type(int len, Type **contained_types) {
  Type *tuple = t_alloc(sizeof(Type));
  tuple->kind = T_CONS;
  tuple->data.T_CONS.name = TYPE_NAME_TUPLE;
  tuple->data.T_CONS.args = contained_types;
  tuple->data.T_CONS.num_args = len;
  return tuple;
}

// Type *create_coroutine_instance_type(Type *ret_type) {
//   Type *coroutine_fn = type_fn(&t_void, create_option_type(ret_type));
//   coroutine_fn->is_coroutine_instance = true;
//   return coroutine_fn;
// }

// Deep copy implementation (simplified)
Type *deep_copy_type(const Type *original) {
  Type *copy = t_alloc(sizeof(Type));
  *copy = *original;
  if (original->closure_meta != NULL) {
    copy->closure_meta = deep_copy_type(original->closure_meta);
  }

  switch (original->kind) {
  case T_VAR:
    copy->data.T_VAR = strdup(original->data.T_VAR);
    break;
  case T_TYPECLASS_RESOLVE:
  case T_CONS:
    // Deep copy of name and args
    copy->data.T_CONS.name = strdup(original->data.T_CONS.name);
    copy->data.T_CONS.num_args = original->data.T_CONS.num_args;
    copy->data.T_CONS.args =
        t_alloc(sizeof(Type *) * copy->data.T_CONS.num_args);

    for (int i = 0; i < copy->data.T_CONS.num_args; i++) {
      copy->data.T_CONS.args[i] = deep_copy_type(original->data.T_CONS.args[i]);
    }
    copy->data.T_CONS.names = original->data.T_CONS.names;

    break;
  case T_FN:
    copy->data.T_FN.from = deep_copy_type(original->data.T_FN.from);
    copy->data.T_FN.to = deep_copy_type(original->data.T_FN.to);
    break;
  }
  return copy;
}

bool is_list_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_LIST) == 0);
}

bool is_forall_type(Type *type) {
  return type->kind == T_CONS &&
         (strncmp(type->data.T_CONS.name, "forall", 6) == 0);
}

bool is_string_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_ARRAY) == 0) &&
         (type->data.T_CONS.args[0]->kind == T_CHAR);
}

bool is_pointer_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_PTR) == 0);
}

bool is_array_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_ARRAY) == 0);
}
bool is_tuple_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0);
}

bool is_sum_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_VARIANT) == 0);
}
Type *resolve_tc_rank(Type *type) {
  // TODO: implement this
  return NULL;
}

Type *resolve_type_in_env(Type *r, TypeEnv *env) {
  if (r->closure_meta) {
    r->closure_meta = resolve_type_in_env(r->closure_meta, env);
  }
  switch (r->kind) {
  case T_VAR: {
    Type *rr = env_lookup(env, r->data.T_VAR);
    if (rr && rr->kind == T_VAR) {
      return resolve_type_in_env(rr, env);
    }

    if (rr) {
      *r = *rr;
    }

    return r;
  }

  case T_TYPECLASS_RESOLVE: {
    bool still_generic = false;
    for (int i = 0; i < r->data.T_CONS.num_args; i++) {
      r->data.T_CONS.args[i] = resolve_type_in_env(r->data.T_CONS.args[i], env);
      if (r->data.T_CONS.args[i]->kind == T_VAR) {
        still_generic = true;
      }
    }
    if (!still_generic) {
      return resolve_tc_rank(r);
    }
    return r;
  }
  case T_CONS: {
    for (int i = 0; i < r->data.T_CONS.num_args; i++) {
      r->data.T_CONS.args[i] = resolve_type_in_env(r->data.T_CONS.args[i], env);
    }
    return r;
  }

  case T_FN: {
    r->data.T_FN.from = resolve_type_in_env(r->data.T_FN.from, env);
    r->data.T_FN.to = resolve_type_in_env(r->data.T_FN.to, env);
    return r;
  }

  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING: {
    return r;
  }
  }
  return NULL;
}

Type *create_cons_type(const char *name, int len, Type **unified_args) {
  Type *cons = empty_type();
  cons->kind = T_CONS;
  cons->data.T_CONS.name = name;
  cons->data.T_CONS.num_args = len;
  cons->data.T_CONS.args = unified_args;
  return cons;
}

TypeClass _GenericEq = {.name = TYPE_NAME_TYPECLASS_EQ, .rank = 1000.};

Type *create_option_type(Type *option_of) {
  Type **variant_members = t_alloc(sizeof(Type *) * 2);

  Type **contained = t_alloc(sizeof(Type *));
  contained[0] = option_of;
  variant_members[0] = create_cons_type(TYPE_NAME_SOME, 1, contained);

  variant_members[1] = create_cons_type(TYPE_NAME_NONE, 0, NULL);
  Type *cons = create_cons_type(TYPE_NAME_VARIANT, 2, variant_members);
  cons->alias = "Option";
  typeclasses_extend(cons, &_GenericEq);
  // printf("created option of \n");
  // print_type(option_of);
  // print_type(cons);
  return cons;
}

Type *ptr_of_type(Type *pointee) {
  Type *ptr = empty_type();
  ptr->kind = T_CONS;
  ptr->alias = TYPE_NAME_PTR;
  ptr->data.T_CONS.name = TYPE_NAME_PTR;
  ptr->data.T_CONS.args = t_alloc(sizeof(Type *));
  ptr->data.T_CONS.args[0] = pointee;
  ptr->data.T_CONS.num_args = 1;
  return ptr;
}

Type *create_array_type(Type *of) {
  Type *gen_array = empty_type();
  gen_array->kind = T_CONS;
  gen_array->data.T_CONS.name = TYPE_NAME_ARRAY;
  gen_array->data.T_CONS.args = t_alloc(sizeof(Type *));
  gen_array->data.T_CONS.num_args = 1;
  gen_array->data.T_CONS.args[0] = of;
  return gen_array;
}

Type *create_list_type_of_type(Type *of) {
  Type *gen_list = empty_type();
  gen_list->kind = T_CONS;
  gen_list->data.T_CONS.name = TYPE_NAME_LIST;
  gen_list->data.T_CONS.args = t_alloc(sizeof(Type *));
  gen_list->data.T_CONS.num_args = 1;
  gen_list->data.T_CONS.args[0] = of;
  return gen_list;
}

int get_struct_member_idx(const char *member_name, Type *type) {
  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    char *n = type->data.T_CONS.names[i];
    if (strcmp(member_name, n) == 0) {
      return i;
    }
  }
  return -1;
}

Type *get_struct_member_type(const char *member_name, Type *type) {
  int idx = get_struct_member_idx(member_name, type);
  if (idx >= 0) {
    return type->data.T_CONS.args[idx];
  }
  return NULL;
}

Type *concat_struct_types(Type *a, Type *b) {
  if (a->kind == T_VOID) {
    return b;
  }

  if (b->kind == T_VOID) {
    return a;
  }

  if (a->kind != T_CONS) {
    Type *cont[] = {a};
    a = create_tuple_type(1, cont);
  }

  if (b->kind != T_CONS) {

    Type *cont[] = {b};
    b = create_tuple_type(1, cont);
  }

  if (strcmp(a->data.T_CONS.name, b->data.T_CONS.name) != 0) {
    return NULL;
  }
  if (a->data.T_CONS.names != NULL) {
    if (b->data.T_CONS.names == NULL) {
      return NULL;
    }
  }

  int lena = a->data.T_CONS.num_args;
  int lenb = b->data.T_CONS.num_args;
  int len = lena + lenb;
  Type **args = t_alloc(sizeof(Type *) * len);
  char **names = NULL;
  if (a->data.T_CONS.names) {
    names = t_alloc(sizeof(char *) * len);
  }

  int i;
  for (i = 0; i < lena; i++) {
    args[i] = a->data.T_CONS.args[i];
    if (names) {
      names[i] = a->data.T_CONS.names[i];
    }
  }
  for (; i < len; i++) {
    args[i] = b->data.T_CONS.args[i - lena];
    if (names) {
      // TODO: fail if duplicate keys used
      names[i] = b->data.T_CONS.names[i - lena];
    }
  }

  Type *concat = empty_type();
  concat->kind = T_CONS;
  concat->data.T_CONS.name = a->data.T_CONS.name;
  concat->data.T_CONS.args = args;
  concat->data.T_CONS.num_args = len;
  concat->data.T_CONS.names = names;
  return concat;
}

TypeClass *get_typeclass_by_name(Type *t, const char *name) {
  for (TypeClass *tc = t->implements; tc; tc = tc->next) {
    if (strcmp(name, tc->name) == 0) {
      return tc;
    }
  }
  return NULL;
}

bool type_implements(Type *t, TypeClass *constraint_tc) {
  if (t->kind == T_TYPECLASS_RESOLVE &&
      (strcmp(t->data.T_CONS.name, constraint_tc->name) == 0)) {
    return true;
  }
  if (!t->implements) {
    return false;
  }
  for (TypeClass *tc = t->implements; tc != NULL; tc = tc->next) {

    if (strcmp(tc->name, constraint_tc->name) == 0) {
      return true;
    }
  }
  return false;
}

double get_typeclass_rank(Type *t, const char *name) {
  TypeClass *tc = get_typeclass_by_name(t, name);
  if (!tc) {
    return -1.;
  }
  return tc->rank;
}

bool is_simple_enum(Type *t) {
  if (t->kind != T_CONS) {
    return false;
  }

  if (strcmp(t->data.T_CONS.name, TYPE_NAME_VARIANT) != 0) {
    return false;
  }

  for (int i = 0; i < t->data.T_CONS.num_args; i++) {
    Type *mem_type = t->data.T_CONS.args[i];
    if (mem_type->data.T_CONS.num_args > 0) {
      return false;
    }
  }
  return true;
}

/**
 * compares two function types for equality, ignoring the return type of each
 * */
bool fn_types_match(Type *t1, Type *t2) {
  while (t1->kind == T_FN) {
    Type *c1 = t1->data.T_FN.from;
    Type *c2 = t2->data.T_FN.from;
    if (!types_equal(c1, c2)) {
      return false;
    }

    t1 = t1->data.T_FN.to;
    t2 = t2->data.T_FN.to;
  }
  return true;
}

bool application_is_partial(Ast *app) {

  if (((Type *)app->data.AST_APPLICATION.function->md)->kind != T_FN) {
    return false;
  }

  int expected_args_len =
      fn_type_args_len(app->data.AST_APPLICATION.function->md);

  int actual_args_len = app->data.AST_APPLICATION.len;
  return actual_args_len < expected_args_len;
}

bool is_coroutine_type(Type *fn_type) {
  return fn_type->kind == T_FN && fn_type->is_coroutine_instance;
}

bool is_coroutine_constructor_type(Type *fn_type) {
  return fn_type->kind == T_FN && fn_type->is_coroutine_constructor;
}

bool is_void_func(Type *f) {
  return (f->kind == T_FN) && (f->data.T_FN.from->kind == T_VOID);
}

TypeClass *impls_extend(TypeClass *impls, TypeClass *tc) {
  tc->next = impls;
  return tc;
}

void typeclasses_extend(Type *t, TypeClass *tc) {
  if (!type_implements(t, tc)) {
    t->implements = impls_extend(t->implements, tc);
  }
}

bool is_module(Type *t) {
  return t->kind == T_CONS &&
         (strcmp(t->data.T_CONS.name, TYPE_NAME_MODULE) == 0);
}

bool is_closure(Type *type) { return type->closure_meta != NULL; }

Type *resolve_tc_rank_in_env(Type *type, TypeEnv *env) {
  // TODO: implement
  return NULL;
}
Type *type_of_option(Type *opt) {
  return opt->data.T_CONS.args[0]->data.T_CONS.args[0];
}
bool is_option_type(Type *opt) {
  // TODO: implement
  return false;
}

Type *create_tc_resolve(TypeClass *tc, Type *t1, Type *t2) {
  if (types_equal(t1, t2)) {
    return t1;
  }
  Type **args = t_alloc(sizeof(Type *) * 2);
  args[0] = t1;
  args[1] = t2;
  Type *resolution = t_alloc(sizeof(Type));
  *resolution =
      (Type){T_TYPECLASS_RESOLVE,
             {.T_CONS = {.name = tc->name, .args = args, .num_args = 2}}};
  resolution->implements = tc;
  return resolution;
}
