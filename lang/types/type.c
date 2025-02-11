#include "type.h"
#include "types/inference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *create_option_type(Type *option_of);

Type t_int = {T_INT};

Type t_uint64 = {T_UINT64};

Type t_num = {T_NUM};

Type t_string = {T_CONS,
                 {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){&t_char}, 1}},
                 .alias = TYPE_NAME_STRING};

Type t_string_add_fn_sig = MAKE_FN_TYPE_3(&t_string, &t_string, &t_string);
Type t_char_array = {T_CONS,
                     {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){&t_char}, 1}}};

Type t_bool = {T_BOOL};

Type t_void = {T_VOID};
Type t_char = {T_CHAR};
Type t_empty_list = {T_EMPTY_LIST};
Type t_ptr = {T_CONS,
              {.T_CONS = {.name = TYPE_NAME_PTR, .num_args = 0}},
              .alias = TYPE_NAME_PTR};

Type t_ptr_generic_contained = {T_VAR, {.T_VAR = "ptr_deref_var"}};
Type t_ptr_generic = {
    T_CONS,
    {.T_CONS = {TYPE_NAME_PTR, (Type *[]){&t_ptr_generic_contained}, 1}}};

Type t_ptr_deref_sig = MAKE_FN_TYPE_2(&t_ptr_generic, &t_ptr_generic_contained);

#define MAKE_ARITH_OP(_name)                                                   \
  Type _name##_a = arithmetic_var("a");                                        \
  Type _name##_b = arithmetic_var("b");                                        \
  Type _name##_res = MAKE_TC_RESOLVE_2("arithmetic", &_name##_a, &_name##_b);  \
  Type _name = MAKE_FN_TYPE_3(&_name##_a, &_name##_b, &_name##_res)

MAKE_ARITH_OP(t_add);
MAKE_ARITH_OP(t_sub);
MAKE_ARITH_OP(t_mul);
MAKE_ARITH_OP(t_div);
MAKE_ARITH_OP(t_mod);

#define MAKE_ORD_OP(_name)                                                     \
  Type _name##_a = ord_var("a");                                               \
  Type _name##_b = ord_var("b");                                               \
  Type _name = MAKE_FN_TYPE_3(&_name##_a, &_name##_b, &t_bool)

MAKE_ORD_OP(t_lt);
MAKE_ORD_OP(t_gt);
MAKE_ORD_OP(t_lte);
MAKE_ORD_OP(t_gte);

#define MAKE_EQ_OP(_name)                                                      \
  Type _name##_a = eq_var("a");                                                \
  Type _name##_b = eq_var("b");                                                \
  Type _name = MAKE_FN_TYPE_3(&_name##_a, &_name##_b, &t_bool)

MAKE_EQ_OP(t_eq);
MAKE_EQ_OP(t_neq);

Type t_bool_binop = MAKE_FN_TYPE_3(&t_bool, &t_bool, &t_bool);

Type t_list_var_el = {T_VAR, {.T_VAR = "vlist_el"}};
Type t_list_var = {
    T_CONS,
    {.T_CONS = {TYPE_NAME_LIST, (Type *[]){&t_list_var_el}, 1}},
};

Type t_list_prepend = MAKE_FN_TYPE_3(&t_list_var_el, &t_list_var, &t_list_var);

Type t_queue_var = {
    T_CONS,
    {.T_CONS = {TYPE_NAME_QUEUE, (Type *[]){&t_list_var_el}, 1}},
};

Type t_queue_of_list = MAKE_FN_TYPE_2(&t_list_var, &t_queue_var);

Type t_queue_pop_left = MAKE_FN_TYPE_2(&t_queue_var, &TOPT(&t_list_var_el));
Type t_queue_append_right =
    MAKE_FN_TYPE_3(&t_queue_var, &t_list_var_el, &t_queue_var);

Type t_list_concat = MAKE_FN_TYPE_3(&t_list_var, &t_list_var, &t_list_var);

_binop_map binop_map[_NUM_BINOPS] = {
    {TYPE_NAME_OP_ADD, &t_add}, {TYPE_NAME_OP_SUB, &t_sub},
    {TYPE_NAME_OP_MUL, &t_mul}, {TYPE_NAME_OP_DIV, &t_div},
    {TYPE_NAME_OP_MOD, &t_mod}, {TYPE_NAME_OP_LT, &t_lt},
    {TYPE_NAME_OP_GT, &t_gt},   {TYPE_NAME_OP_LTE, &t_lte},
    {TYPE_NAME_OP_GTE, &t_gte}, {TYPE_NAME_OP_EQ, &t_eq},
    {TYPE_NAME_OP_NEQ, &t_neq}, {TYPE_NAME_OP_LIST_PREPEND, &t_list_prepend},
};

//
static char *type_name_mapping[] = {
    [T_INT] = TYPE_NAME_INT,    [T_UINT64] = TYPE_NAME_UINT64,
    [T_NUM] = TYPE_NAME_DOUBLE, [T_BOOL] = TYPE_NAME_BOOL,
    [T_VOID] = TYPE_NAME_VOID,  [T_CHAR] = TYPE_NAME_CHAR,
};

char *tc_list_to_string(Type *t, char *buffer) {
  if (t->implements != NULL) {
    buffer = strncat(buffer, " [", 2);
    for (TypeClass *tc = t->implements; tc != NULL; tc = tc->next) {
      buffer = strncat(buffer, tc->name, strlen(tc->name));
      buffer = strncat(buffer, ", ", 2);
    }
    buffer = strncat(buffer, "]", 1);
  }
  return buffer;
}

Type t_array_var_el = {T_VAR, {.T_VAR = "varray_el"}};

Type t_array_var = {
    T_CONS,
    {.T_CONS = {TYPE_NAME_ARRAY, (Type *[]){&t_array_var_el}, 1}},
};

Type t_array_size_fn_sig = MAKE_FN_TYPE_2(&t_array_var, &t_int);

Type t_array_data_ptr_fn_sig = MAKE_FN_TYPE_2(&t_array_var, &t_ptr);

Type t_array_incr_fn_sig = MAKE_FN_TYPE_2(&t_array_var, &t_array_var);
Type t_array_slice_fn_sig =
    MAKE_FN_TYPE_4(&t_int, &t_int, &t_array_var, &t_array_var);

Type t_array_new_fn_sig = MAKE_FN_TYPE_3(&t_int, &t_array_var_el, &t_array_var);

Type t_array_to_list_fn_sig =
    MAKE_FN_TYPE_2(&t_array_var, &TLIST(&t_array_var_el));
// , &(Type){
//   T_CONS, {.T_CONS = {TYPE_NAME_LIST, (Type *[]){&t_array_var_el}}});

Type *create_new_array_at_sig(void *i) {
  Type *t = next_tvar();
  Type *f = t;
  f = type_fn(&t_int, f);
  f = type_fn(create_array_type(t, 0), f);
  return f;
}

// Type t_array_at_fn_sig = {T_CREATE_NEW_GENERIC,
//                           {.T_CREATE_NEW_GENERIC = create_new_array_at_sig}};

Type t_array_at_fn_sig = MAKE_FN_TYPE_3(&t_array_var, &t_int, &t_array_var_el);

Type t_array_set_fn_sig =
    MAKE_FN_TYPE_4(&t_int, &t_array_var, &t_array_var_el, &t_array_var);

Type t_array_of_chars_fn_sig = MAKE_FN_TYPE_2(&t_string, &t_char_array);

Type t_ref_type = {T_VAR, {.T_VAR = "t_ref_type"}};
Type t_make_ref = MAKE_FN_TYPE_2(&t_ref_type, &TARRAY(&t_ref_type));

Type t_for_sig =
    MAKE_FN_TYPE_4(&t_int, &t_int, &MAKE_FN_TYPE_2(&t_int, &t_void), &t_void);

Type t_option_var = {T_VAR, {.T_VAR = "t"}};
Type t_none = {T_CONS, {.T_CONS = {"None", NULL, 0}}};

bool is_option_type(Type *t) {
  return t->kind == T_CONS && ((strcmp(t->data.T_CONS.name, "Some") == 0) ||
                               (strcmp(t->data.T_CONS.name, "None") == 0));
}

// Type t_option_of_var =
//     TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, &t_option_var), &t_none);

Type *create_new_option_of_var() { return create_option_type(next_tvar()); }

Type t_option_of_var = {T_CREATE_NEW_GENERIC,
                        {.T_CREATE_NEW_GENERIC = create_new_option_of_var}};

// TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, &t_option_var), &t_none);

Type *type_of_option(Type *option) {
  return option->data.T_CONS.args[0]->data.T_CONS.args[0];
}

Type t_cor_wrap_ret_type = {T_VAR, {.T_VAR = "tt"}};
Type t_cor_wrap_state_type = {T_VAR, {.T_VAR = "xx"}};

// Type t_cor_wrap = {
//     T_CONS,
//     {.T_CONS = {
//          "coroutine",
//          (Type *[]){&t_cor_wrap_state_type,
//                     &MAKE_FN_TYPE_2(&t_void, &TOPT(&t_cor_wrap_ret_type))},
//          2}}};
//
Type t_cor_wrap = {T_FN,
                   {.T_FN =
                        {
                            .from = &t_void,
                            .to = &TOPT(&t_cor_wrap_ret_type),
                        }},
                   .is_coroutine_instance = true};

Type t_cor_wrap_effect_fn_sig = MAKE_FN_TYPE_3(
    &MAKE_FN_TYPE_2(&t_cor_wrap_ret_type, &t_void), &t_cor_wrap, &t_cor_wrap);

Type t_cor_map_from_type = {T_VAR, {.T_VAR = "map_from"}};

Type t_cor_map_to_type = {T_VAR, {.T_VAR = "map_to"}};

Type t_cor_from = {
    T_FN,
    {.T_FN = {.from = &t_void, .to = &TOPT(&t_cor_map_from_type)}},
    .is_coroutine_instance = true};

Type t_cor_to = {T_FN,
                 {.T_FN = {.from = &t_void, .to = &TOPT(&t_cor_map_to_type)}},
                 .is_coroutine_instance = true};

Type t_cor_map_fn_sig =
    MAKE_FN_TYPE_3(&MAKE_FN_TYPE_2(&t_cor_map_from_type, &t_cor_map_to_type),
                   &t_cor_from, &t_cor_to);

Type t_cor_loop_var = {
    T_FN,
    {.T_FN = {.from = &t_void, .to = &TOPT(&t_cor_map_from_type)}},
    .is_coroutine_instance = true};

Type t_cor_loop_sig = MAKE_FN_TYPE_2(&t_cor_loop_var, &t_cor_loop_var);

Type t_cor_play_sig = MAKE_FN_TYPE_3(&t_ptr, // schedule event injected func
                                     &t_cor_loop_var, &t_void);

Type t_list_cor = {T_FN,
                   {.T_FN = {.from = &t_void, .to = &TOPT(&t_list_var_el)}},
                   .is_coroutine_instance = true};
Type t_iter_of_list_sig =
    ((Type){T_FN,
            {.T_FN = {.from = &TLIST(&t_list_var_el), .to = &t_list_cor}},
            .is_coroutine_constructor = true});

Type t_iter_of_array_sig = ((Type){
    T_FN,
    {.T_FN = {.from = &((Type){T_CONS,
                               {.T_CONS = {TYPE_NAME_ARRAY,
                                           (Type *[]){&t_list_var_el}, 1}}}),
              .to = &t_list_cor}},
    .is_coroutine_constructor = true});

/*
Type t_cor_params = TVAR("cor_params");
Type t_cor_ret = TVAR("cor_ret");
Type t_cor_ret_opt =
    TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, &t_cor_ret), &t_none);

Type t_looped_cor_inst = COR_INST(&t_cor_params, &t_cor_ret_opt);
Type t_looped_cor_def = MAKE_FN_TYPE_2(&t_cor_params, &t_looped_cor_inst);
Type t_cor_loop_sig =
    MAKE_FN_TYPE_3(&t_looped_cor_def, &t_cor_params, &t_looped_cor_inst);

Type t_itered_cor_inst = COR_INST(&t_cor_params, &t_cor_ret_opt);
Type t_iter_cor_def = MAKE_FN_TYPE_2(&t_cor_ret, &t_void);

Type t_iter_cor_sig =
    MAKE_FN_TYPE_3(&t_iter_cor_def, &COR_INST(&t_cor_params,
&t_cor_ret_opt), &COR_INST(&t_cor_params, &t_cor_ret_opt));

Type t_array_cor_el = TVAR("array_cor");
Type t_array_cor_ret_opt =
    TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, &t_array_cor_el),
&t_none); Type t_array_cor_params = TARRAY(&t_array_cor_el); Type
t_iter_of_array_sig = MAKE_FN_TYPE_2( &t_array_cor_params,
&COR_INST(&t_array_cor_params, &t_array_cor_ret_opt));

Type t_list_cor_el = TVAR("list_cor");
Type t_list_cor_ret_opt =
    TCONS(TYPE_NAME_VARIANT, 2, &TCONS("Some", 1, &t_list_cor_el), &t_none);
Type t_list_cor_params = TLIST(&t_list_cor_el);
Type t_iter_of_list_sig = MAKE_FN_TYPE_2(
    &t_list_cor_params, &COR_INST(&t_list_cor_params, &t_list_cor_ret_opt));

Type t_coroutine_concat_sig = TVAR("t_coroutine_concat_sig");
*/
Type t_builtin_or = MAKE_FN_TYPE_3(&t_bool, &t_bool, &t_bool);
Type t_builtin_and = MAKE_FN_TYPE_3(&t_bool, &t_bool, &t_bool);

// Type t_cor_map_iter_sig;

char *type_to_string(Type *t, char *buffer) {
  if (t == NULL) {
    return strncat(buffer, "null", 4);
  }

  // if (t->alias != NULL) {
  //   return strncat(buffer, t->alias, strlen(t->alias));
  // }
  //
  switch (t->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_BOOL:
  case T_VOID:
  case T_CHAR: {
    char *m = type_name_mapping[t->kind];
    buffer = strncat(buffer, m, strlen(m));
    break;
  }
  case T_EMPTY_LIST: {

    buffer = strncat(buffer, "[]", 2);
    break;
  }

  case T_TYPECLASS_RESOLVE: {
    buffer = strncat(buffer, "tc resolve ", 12);

    buffer = strncat(buffer, t->data.T_CONS.name, strlen(t->data.T_CONS.name));
    buffer = strncat(buffer, " [ ", 3);

    int len = t->data.T_CONS.num_args;
    for (int i = 0; i < len - 1; i++) {
      buffer = type_to_string(t->data.T_CONS.args[i], buffer);
    }

    buffer = strncat(buffer, " : ", 3);
    buffer = type_to_string(t->data.T_CONS.args[len - 1], buffer);

    buffer = strncat(buffer, "]", 1);
    break;
  }
  case T_CONS: {

    if (is_forall_type(t)) {
      buffer = strncat(buffer, "forall ", 7);
      int len = t->data.T_CONS.num_args;
      for (int i = 0; i < len - 1; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
      }

      buffer = strncat(buffer, " : ", 3);
      buffer = type_to_string(t->data.T_CONS.args[len - 1], buffer);
      break;
    }

    if (is_list_type(t)) {
      buffer = type_to_string(t->data.T_CONS.args[0], buffer);
      buffer = strncat(buffer, "[]", 2);
      break;
    }

    if (is_tuple_type(t)) {
      buffer = strncat(buffer, "( ", 2);
      int is_named = t->data.T_CONS.names != NULL;
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        if (is_named) {
          buffer = strncat(buffer, t->data.T_CONS.names[i],
                           strlen(t->data.T_CONS.names[i]));
          buffer = strncat(buffer, ": ", 2);
        }
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strncat(buffer, " * ", 3);
        }
      }

      buffer = strncat(buffer, " )", 2);
      break;
    }

    if (is_variant_type(t)) {

      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strncat(buffer, " | ", 3);
        }
      }
      break;
    }
    // if (is_array_type(t)) {
    //   buffer = type_to_string(t->data.T_CONS.args[0], buffer);
    //   buffer = strncat(buffer, "[|", 1);
    //   if (t->data.T_CONS.num_args > 1) {
    //     buffer = strncat(buffer, "%d", 2);
    //   }
    //   buffer = strncat(buffer, "|]", 1);
    // }

    buffer = strncat(buffer, t->data.T_CONS.name, strlen(t->data.T_CONS.name));
    if (t->data.T_CONS.num_args > 0) {
      buffer = strncat(buffer, " of ", 4);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strcat(buffer, ", ");
        }
      }
    }
    buffer = tc_list_to_string(t, buffer);
    break;
  }
  case T_VAR: {
    uint64_t vname = (uint64_t)t->data.T_VAR;
    if (vname < 65) {
      vname += 65;
      buffer = strncat(buffer, (char *)&vname, 1);
    } else {

      buffer = strncat(buffer, t->data.T_VAR, strlen(t->data.T_VAR));
    }

    buffer = tc_list_to_string(t, buffer);
    break;
  }

  case T_FN: {
    Type *fn = t;

    buffer = strcat(buffer, "(");
    if (fn->is_coroutine_constructor) {
      buffer = strcat(buffer, "[coroutine constructor]");
    }

    while (fn->kind == T_FN) {
      if (fn->is_coroutine_instance) {
        buffer = strcat(buffer, "[coroutine instance]");
      }
      buffer = type_to_string(fn->data.T_FN.from, buffer);
      buffer = strncat(buffer, " -> ", 4);
      fn = fn->data.T_FN.to;
    }
    // If it's not a function type, it's the return type itself
    buffer = type_to_string(fn, buffer);
    buffer = strcat(buffer, ")");
    break;
  }
  }

  return buffer;
}

void print_type(Type *t) {
  if (!t) {
    printf("null\n");
    return;
  }

  // if (t->alias) {
  //   printf("%s\n", t->alias);
  //   return;
  // }

  char buf[400] = {};
  printf("%s\n", type_to_string(t, buf));
}

void print_type_err(Type *t) {
  if (!t) {
    fprintf(stderr, "null\n");
    return;
  }
  char buf[200] = {};
  fprintf(stderr, "%s", type_to_string(t, buf));
}

bool variant_contains_type(Type *variant, Type *member, int *idx) {
  if (!is_variant_type(variant)) {
    if (is_variant_type(member)) {
      return variant_contains_type(member, variant, idx);
    }
    return false;
  }
  for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
    if (types_equal(member, variant->data.T_CONS.args[i])) {
      if (idx != NULL) {
        *idx = i;
      }

      return true;
    }
  }
  return false;
}

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
  }
  return false;
}

static struct TStorage {
  void *data;
  size_t size;
  size_t capacity;
} TStorage;
static void *_tstorage_data[_TSTORAGE_SIZE_DEFAULT];

static struct TStorage _tstorage = {_tstorage_data, 0, _TSTORAGE_SIZE_DEFAULT};

void *talloc(size_t size) {
  // printf("alloc size %zu\n", size);
  // malloc
  // void *mem = malloc(size);
  // if (!mem) {
  //   fprintf(stderr, "Error allocating memory for type");
  // }
  // return mem;
  if (_tstorage.size + size > _tstorage.capacity) {
    fprintf(stderr, "OOM Error allocating memory for type");
    return NULL;
  }
  void *mem = _tstorage.data + _tstorage.size;
  _tstorage.size += size;
  return mem;
}

void tfree(void *mem) {
  // free(mem);
}

Type *empty_type() {
  Type *mem = talloc(sizeof(Type));
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  return mem;
}

Type *tvar(const char *name) {
  Type *mem = empty_type();
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  mem->kind = T_VAR;
  mem->data.T_VAR = talloc(sizeof(char) * strlen(name));
  memcpy(mem->data.T_VAR, name, strlen(name));
  return mem;
}

Type *fn_return_type(Type *fn) {
  if (fn->kind != T_FN) {
    // If it's not a function type, it's the return type itself
    return fn;
  }
  // Recursively check the 'to' field
  return fn_return_type(fn->data.T_FN.to);
}

bool is_generic(Type *t) {
  if (t == NULL) {
    fprintf(stderr, "Error type passed to generic test is null\n");
    return NULL;
  }

  switch (t->kind) {
  case T_VAR: {
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

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  TypeEnv *new_env = talloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  while (env) {
    if (env->name && strcmp(env->name, name) == 0) {
      return env->type;
    }

    env = env->next;
  }

  return lookup_builtin_type(name);
}

Type *rec_env_lookup(TypeEnv *env, Type *var) {
  while (var && var->kind == T_VAR) {
    var = env_lookup(env, var->data.T_VAR);
  }
  return var;
}

Type *variant_member_lookup(TypeEnv *env, const char *name, int *idx,
                            char **variant_name) {
  while (env) {
    if (is_variant_type(env->type)) {
      Type *variant = env->type;
      const char *_variant_name = env->name;
      for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
        Type *variant_member = variant->data.T_CONS.args[i];
        const char *mem_name;
        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          *idx = i;
          *variant_name = _variant_name;
          return variant;
        }
      }
    }
    if (strcmp(env->name, name) == 0) {
      return env->type;
    }

    env = env->next;
  }
  return NULL;
}

void free_type_env(TypeEnv *env) {
  // if (env->next) {
  //   free_type_env(env->next);
  //   free(env);
  // }
}

void print_type_env(TypeEnv *env) {
  if (!env) {
    return;
  }
  printf("%s : ", env->name);
  print_type(env->type);
  if (env->next) {
    print_type_env(env->next);
  }
}

Type *type_fn(Type *from, Type *to) {
  Type *fn = empty_type();
  fn->kind = T_FN;
  fn->data.T_FN.from = from;
  fn->data.T_FN.to = to;
  fn->is_recursive_fn_ref = false;
  return fn;
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

Type *create_tuple_type(int len, Type **contained_types) {
  Type *tuple = talloc(sizeof(Type));
  tuple->kind = T_CONS;
  tuple->data.T_CONS.name = TYPE_NAME_TUPLE;
  tuple->data.T_CONS.args = contained_types;
  tuple->data.T_CONS.num_args = len;
  return tuple;
}

Type *create_coroutine_instance_type(Type *ret_type) {
  Type *coroutine_fn = type_fn(&t_void, create_option_type(ret_type));
  coroutine_fn->is_coroutine_instance = true;
  return coroutine_fn;
}

// Deep copy implementation (simplified)
Type *deep_copy_type(const Type *original) {
  Type *copy = talloc(sizeof(Type));
  copy->kind = original->kind;
  copy->alias = original->alias;
  copy->constructor = original->constructor;
  copy->implements = original->implements;
  copy->is_recursive_fn_ref = original->is_recursive_fn_ref;
  copy->is_coroutine_constructor = original->is_coroutine_constructor;
  copy->is_coroutine_instance = original->is_coroutine_instance;

  // for (int i = 0; i < original->num_implements; i++) {
  //   add_typeclass(copy, original->implements[i]);
  // }

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
        talloc(sizeof(Type *) * copy->data.T_CONS.num_args);

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

Type *copy_array_type(Type *t) {
  int *size = array_type_size_ptr(t);

  Type *copy = create_array_type(deep_copy_type(t->data.T_CONS.args[0]), *size);
  copy->kind = t->kind;
  return copy;
}

int fn_type_args_len(Type *fn_type) {
  if (fn_type->is_coroutine_constructor) {
    int args_len = 0;
    Type *f = fn_type;
    while (!(f->is_coroutine_instance)) {
      args_len++;
      f = f->data.T_FN.to;
    }
    return args_len;
  }

  if (fn_type->data.T_FN.from->kind == T_VOID) {
    return 1;
  }

  int fn_len = 0;
  Type *t = fn_type;
  while (t->kind == T_FN) {
    // printf("arg %d: ", fn_len);
    // print_type(t->data.T_FN.from);
    t = t->data.T_FN.to;
    fn_len++;
  }

  return fn_len;
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

bool is_variant_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, TYPE_NAME_VARIANT) == 0);
}

Type *create_typeclass_resolve_type(const char *comparison_tc, int num,
                                    Type **types) {
  Type *tcr = empty_type();
  tcr->kind = T_TYPECLASS_RESOLVE;
  tcr->data.T_CONS.name = comparison_tc;
  tcr->data.T_CONS.num_args = num;
  tcr->data.T_CONS.args = types;
  return tcr;
}

Type *resolve_tc_rank(Type *type) {
  if (type->kind != T_TYPECLASS_RESOLVE) {
    return type;
  }
  bool all_generic = true;

  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    all_generic &= is_generic(type->data.T_CONS.args[i]);
  }

  if (all_generic) {
    return type;
  }

  const char *comparison_tc = type->data.T_CONS.name;
  Type *max_ranked = NULL;
  double max_rank;
  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    Type *arg = type->data.T_CONS.args[i];
    if (max_ranked == NULL) {
      max_ranked = arg;
      max_rank = get_typeclass_rank(arg, comparison_tc);
    } else if (get_typeclass_rank(arg, comparison_tc) >= max_rank) {
      max_ranked = arg;
      max_rank = get_typeclass_rank(arg, comparison_tc);
    }
  }
  return max_ranked;
}

Type *resolve_type_in_env(Type *r, TypeEnv *env) {
  switch (r->kind) {
  case T_VAR: {
    Type *rr = env_lookup(env, r->data.T_VAR);
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
// Type *resolve_type_in_env(Type *t, TypeEnv *env) {
//   if (t->kind == T_VAR) {
//     return env_lookup(env, t->data.T_VAR);
//   }
//   if (t->kind == T_CONS || t->kind == T_TYPECLASS_RESOLVE) {
//     Type *cp = deep_copy_type(t);
//     for (int i = 0; i < cp->data.T_CONS.num_args; i++) {
//       cp->data.T_CONS.args[i] =
//           resolve_type_in_env(cp->data.T_CONS.args[i], env);
//     }
//     return cp;
//   }
//
//   return t;
// }

Type *resolve_tc_rank_in_env(Type *type, TypeEnv *env) {
  if (type->kind != T_TYPECLASS_RESOLVE) {
    return type;
  }

  const char *comparison_tc = type->data.T_CONS.name;
  Type *max_ranked = NULL;
  double max_rank;
  for (int i = 0; i < type->data.T_CONS.num_args; i++) {
    Type *arg = type->data.T_CONS.args[i];
    arg = resolve_type_in_env(arg, env);

    if (max_ranked == NULL) {
      max_ranked = arg;
      max_rank = get_typeclass_rank(arg, comparison_tc);
    } else if (get_typeclass_rank(arg, comparison_tc) >= max_rank) {
      max_ranked = arg;
      max_rank = get_typeclass_rank(arg, comparison_tc);
    }
  }
  return max_ranked;
}

Type *replace_in(Type *type, Type *tvar, Type *replacement) {
  switch (type->kind) {

  case T_CONS: {

    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      type->data.T_CONS.args[i] =
          replace_in(type->data.T_CONS.args[i], tvar, replacement);
    }
    return type;
  }
  case T_FN: {
    type->data.T_FN.from = replace_in(type->data.T_FN.from, tvar, replacement);
    type->data.T_FN.to = replace_in(type->data.T_FN.to, tvar, replacement);
    return type;
  }

  case T_VAR: {
    if (strcmp(type->data.T_VAR, tvar->data.T_VAR) == 0) {
      return replacement;
    }
    return type;
  }

  default:
    return type;
  }
}
Type *resolve_generic_type(Type *t, TypeEnv *env) {
  while (env) {
    const char *key = env->name;
    Type tvar = {T_VAR, .data = {.T_VAR = key}};
    t = replace_in(t, &tvar, env->type);
    env = env->next;
  }

  return t;
}

Type *variant_lookup(TypeEnv *env, Type *member, int *member_idx) {
  const char *name = member->data.T_CONS.name;

  while (env) {
    if (is_variant_type(env->type)) {
      Type *variant = env->type;
      for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
        Type *variant_member = variant->data.T_CONS.args[i];
        const char *mem_name;
        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          *member_idx = i;
          // printf("found member idx: %d\n", *member_idx);
          return variant;
        }
      }
    }

    env = env->next;
  }
  return NULL;
}

Type *variant_lookup_name(TypeEnv *env, const char *name, int *member_idx) {
  while (env) {
    if (is_variant_type(env->type)) {
      Type *variant = env->type;
      for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
        Type *variant_member = variant->data.T_CONS.args[i];
        const char *mem_name;
        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          // return copy_type(variant);
          *member_idx = i;
          return variant;
        }
      }
    }

    env = env->next;
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

Type *create_option_type(Type *option_of) {
  Type **variant_members = talloc(sizeof(Type *) * 2);

  Type **contained = talloc(sizeof(Type *));
  contained[0] = option_of;
  variant_members[0] = create_cons_type(TYPE_NAME_SOME, 1, contained);

  variant_members[1] = create_cons_type(TYPE_NAME_NONE, 0, NULL);
  Type *cons = create_cons_type(TYPE_NAME_VARIANT, 2, variant_members);
  cons->alias = "Option";
  return cons;
}

Type *ptr_of_type(Type *pointee) {
  Type *ptr = empty_type();
  ptr->kind = T_CONS;
  ptr->alias = TYPE_NAME_PTR;
  ptr->data.T_CONS.name = TYPE_NAME_PTR;
  ptr->data.T_CONS.args = talloc(sizeof(Type *));
  ptr->data.T_CONS.args[0] = pointee;
  ptr->data.T_CONS.num_args = 1;
  return ptr;
}

int *array_type_size_ptr(Type *t) {
  if (!is_array_type(t)) {
    return NULL;
  }
  void *data = t->data.T_CONS.args;
  int *size = t->meta;
  return size;
}

Type *create_array_type(Type *of, int size) {
  Type *gen_array = empty_type();
  gen_array->kind = T_CONS;
  gen_array->data.T_CONS.name = TYPE_NAME_ARRAY;
  // gen_array->data.T_CONS.args = talloc(sizeof(Type *) + sizeof(int));
  gen_array->data.T_CONS.args = talloc(sizeof(Type *));
  gen_array->data.T_CONS.num_args = 1;
  gen_array->data.T_CONS.args[0] = of;
  return gen_array;
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
  Type **args = talloc(sizeof(Type *) * len);
  char **names = NULL;
  if (a->data.T_CONS.names) {
    names = talloc(sizeof(char *) * len);
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

Type t_builtin_print = MAKE_FN_TYPE_2(&t_string, &t_void);

Type t_builtin_char_of = MAKE_FN_TYPE_2(&t_int, &t_char);

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
