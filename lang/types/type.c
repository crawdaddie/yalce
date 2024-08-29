#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type t_int = {T_INT, .num_implements = 3,
              .implements = (TypeClass *[]){
                  &TCEq_int,
                  &TCOrd_int,
                  &TCArithmetic_int,
              }};

Type t_uint64 = {T_UINT64, .num_implements = 3,
                 .implements = (TypeClass *[]){
                     &TCEq_uint64,
                     &TCOrd_uint64,
                     &TCArithmetic_uint64,
                 }};

Type t_num = {T_NUM, .num_implements = 3,
              .implements = (TypeClass *[]){
                  &TCEq_num,
                  &TCOrd_num,
                  &TCArithmetic_num,
              }};

Type t_string;
Type t_bool = {T_BOOL};
Type t_void = {T_VOID};
Type t_char = {T_CHAR};
Type t_ptr = {T_CONS};

static char *type_name_mapping[] = {
    [T_INT] = TYPE_NAME_INT,    [T_UINT64] = TYPE_NAME_UINT64,
    [T_NUM] = TYPE_NAME_DOUBLE, [T_BOOL] = TYPE_NAME_BOOL,
    [T_VOID] = TYPE_NAME_VOID,  [T_CHAR] = TYPE_NAME_CHAR,
};
char *tc_list_to_string(Type *t, char *buffer) {
  if (t->num_implements > 0 && t->implements != NULL) {
    buffer = strncat(buffer, " [", 2);
    for (int i = 0; i < t->num_implements; i++) {
      buffer = strncat(buffer, t->implements[i]->name,
                       strlen(t->implements[i]->name));
      if (i != t->num_implements - 1) {
        buffer = strncat(buffer, ", ", 2);
      }
    }
    buffer = strncat(buffer, "]", 1);
  }
  return buffer;
}

char *type_to_string(Type *t, char *buffer) {
  if (t == NULL) {
    buffer = strncat(buffer, "null", 4);
  }
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
  case T_CONS: {

    if (strcmp(t->data.T_CONS.name, "List") == 0) {
      buffer = type_to_string(t->data.T_CONS.args[0], buffer);
      buffer = strncat(buffer, "[]", 2);
      break;
    }

    if (strcmp(t->data.T_CONS.name, "Tuple") == 0) {
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strncat(buffer, " * ", 3);
        }
      }
      break;
    }

    if (strcmp(t->data.T_CONS.name, "Variant") == 0) {
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strncat(buffer, " | ", 3);
        }
      }
      break;
    }

    buffer = strncat(buffer, t->data.T_CONS.name, strlen(t->data.T_CONS.name));
    if (t->data.T_CONS.args > 0) {
      buffer = strncat(buffer, " of ", 4);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
      }
    }
    buffer = tc_list_to_string(t, buffer);
    break;
  }
  case T_VAR: {
    buffer = strncat(buffer, t->data.T_VAR, strlen(t->data.T_VAR));
    buffer = tc_list_to_string(t, buffer);
    break;
  }
  case T_FN: {
    Type *fn = t;
    while (fn->kind == T_FN) {
      buffer = type_to_string(fn->data.T_FN.from, buffer);
      buffer = strncat(buffer, " -> ", 4);
      fn = fn->data.T_FN.to;
    }
    // If it's not a function type, it's the return type itself
    buffer = type_to_string(fn, buffer);
    break;
  }
  }

  return buffer;
}

void print_type(Type *t) {
  if (!t) {
    return;
  }
  char buf[200] = {};
  printf("%s\n", type_to_string(t, buf));
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
  case T_VOID: {
    return true;
  }

  case T_VAR: {

    bool eq = strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0;
    if (t2->num_implements > 0) {

      if (t1->num_implements == 0 || t1->implements == NULL) {
        eq &= false;
      }
      for (int i = 0; i < t2->num_implements; i++) {
        eq &= implements(t1, t2->implements[i]);
      }
    }
    return eq;
  }

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
      return types_equal(t1->data.T_FN.to, t2->data.T_FN.to);
    }
    return false;
  }
    // case T_VARIANT: {
    //   if (t1->data.T_VARIANT.num_args != t2->data.T_VARIANT.num_args) {
    //     return false;
    //   }
    //   for (int i = 0; i < t1->data.T_VARIANT.num_args; i++) {
    //     if (!types_equal(t1->data.T_VARIANT.args[i],
    //                      t2->data.T_VARIANT.args[i])) {
    //       return false;
    //     }
    //   }
    //   return true;
    // }
  }
  return false;
}

void *talloc(size_t size) {
  void *mem = malloc(size);
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  return mem;
}

void tfree(void *mem) { free(mem); }

Type *empty_type() {
  Type *mem = malloc(sizeof(Type));
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  return mem;
}

Type *tvar(const char *name) {
  Type *mem = malloc(sizeof(Type));
  if (!mem) {
    fprintf(stderr, "Error allocating memory for type");
  }
  *mem = (Type){T_VAR, {.T_VAR = strdup(name)}};
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
  switch (t->kind) {
  case T_VAR: {
    return true;
  }
  case T_CONS: {
    for (int i = 0; i < t->data.T_CONS.num_args; i++) {
      if (is_generic(t->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  }

  case T_FN: {
    if (!is_generic(t->data.T_FN.from)) {
      return is_generic(t->data.T_FN.to);
    }
    return false;
  }
  default:
    return false;
  }
}

TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type) {
  TypeEnv *new_env = malloc(sizeof(TypeEnv));
  new_env->name = name;
  new_env->type = type;
  new_env->next = env;
  return new_env;
}

Type *env_lookup(TypeEnv *env, const char *name) {
  while (env) {
    if (strcmp(env->name, name) == 0) {
      return env->type;
    }
    if (env->type->kind == T_CONS &&
        strcmp(env->type->data.T_CONS.name, "Variant") == 0) {
      Type *variant = env->type;
      for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
        Type *variant_member = variant->data.T_CONS.args[i];
        print_type(variant_member);
        const char *mem_name;

        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else if (variant_member->kind == T_VAR) {
          mem_name = variant_member->data.T_VAR;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          return variant_member;
        }
      }
    }

    if (env->type->kind == T_FN && env->type->data.T_FN.to->kind == T_CONS &&
        strcmp(env->type->data.T_FN.to->data.T_CONS.name, "Variant") == 0) {

      // printf("check variant fn in env\n");
      Type *variant = env->type->data.T_FN.to;
      for (int i = 0; i < variant->data.T_CONS.num_args; i++) {
        Type *variant_member = variant->data.T_CONS.args[i];
        const char *mem_name;

        if (variant_member->kind == T_CONS) {
          mem_name = variant_member->data.T_CONS.name;
        } else if (variant_member->kind == T_VAR) {
          mem_name = variant_member->data.T_VAR;
        } else {
          continue;
        }

        if (strcmp(mem_name, name) == 0) {
          Type *to = empty_type();
          to->kind = T_CONS;
          to->data.T_CONS.name = strdup(mem_name);
          to->data.T_CONS.args = talloc(sizeof(Type *));
          to->data.T_CONS.args[0] = env->type->data.T_FN.from;
          to->data.T_CONS.num_args = 1;
          Type *f = type_fn(env->type->data.T_FN.from, to);
          return f;
        }
      }
    }
    env = env->next;
  }
  return NULL;
}
Type *get_builtin_type(const char *id_chars) {

  if (strcmp(id_chars, TYPE_NAME_INT) == 0) {
    return &t_int;
  } else if (strcmp(id_chars, TYPE_NAME_DOUBLE) == 0) {
    return &t_num;
  } else if (strcmp(id_chars, TYPE_NAME_UINT64) == 0) {
    return &t_uint64;
  } else if (strcmp(id_chars, TYPE_NAME_BOOL) == 0) {
    return &t_bool;
  } else if (strcmp(id_chars, TYPE_NAME_STRING) == 0) {
    return &t_string;
  } else if (strcmp(id_chars, TYPE_NAME_PTR) == 0) {
    return &t_ptr;
  }
  // fprintf(stderr, "Error: type or typeclass %s not found\n", id_chars);

  return NULL;
}

Type *find_type_in_env(TypeEnv *env, const char *name) {
  Type *_type = env_lookup(env, name);

  if (!_type) {
    _type = get_builtin_type(name);
    if (!_type) {
      return NULL;
    }
  }
  return _type;
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
  return fn;
}
