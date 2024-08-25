#include "type.h"
#include "common.h"
#include "parse.h"
#include "typeclass.h"
#include <stdio.h>
#include <string.h>

static char *type_name_mapping[] = {
    [T_INT] = TYPE_NAME_INT,    [T_UINT64] = TYPE_NAME_UINT64,
    [T_NUM] = TYPE_NAME_DOUBLE, [T_BOOL] = TYPE_NAME_BOOL,
    [T_VOID] = TYPE_NAME_VOID,  [T_CHAR] = TYPE_NAME_CHAR,
};

#define ADD_IMPL(tcs, num) .implements = tcs, .num_implements = num

// clang-format off
Type t_int =    {T_INT,     ADD_IMPL(int_tc_table,     11)};
Type t_uint64 = {T_UINT64,  ADD_IMPL(uint64_tc_table,  11)};
Type t_num =    {T_NUM,     ADD_IMPL(num_tc_table,     11)};
Type t_bool =   {T_BOOL};
Type t_void =   {T_VOID};
Type t_char =   {T_CHAR};
Type t_string;
Type t_ptr;

bool types_equal(Type a, Type b) {
  if (a.kind != b.kind) {
    return false;
  }

  switch (a.kind) {
    case T_INT:
    case T_UINT64:
    case T_NUM:
    case T_BOOL:
    case T_VOID:
    case T_CHAR: {
      return true;
    }
    case T_FN: {
      return (
        types_equal(*a.data.T_FN.from, *b.data.T_FN.from) &&
        types_equal(*a.data.T_FN.to, *b.data.T_FN.to)
      );
    }
    case T_CONS: {
      if (!(strcmp(a.data.T_CONS.name, b.data.T_CONS.name) == 0)) {
        return false;
      }

      if (a.data.T_CONS.num_args != b.data.T_CONS.num_args) {
        return false;
      }

      for (int i = 0; i < a.data.T_CONS.num_args; i++) {
        if (!types_equal(a.data.T_CONS.args[i], b.data.T_CONS.args[i])) {
          return false;
        }
      }
      return true;
    }
  }

  return false;
}

char *type_to_string(Type t, char *buffer) {
  switch (t.kind) {
    case T_INT:
    case T_UINT64:
    case T_NUM:
    case T_BOOL:
    case T_VOID:
    case T_CHAR: {
      char *m = type_name_mapping[t.kind];
      buffer = strncat(buffer, m, strlen(m)); 
      break;
    }
    case T_CONS: {

      if (strcmp(t.data.T_CONS.name, "List") == 0) {
        buffer = type_to_string(t.data.T_CONS.args[0], buffer);
        buffer = strcat(buffer, "[]");
        break;
      }

      if (strcmp(t.data.T_CONS.name, "Tuple") == 0) {
        for (int i = 0; i < t.data.T_CONS.num_args; i++) {
          buffer = type_to_string(t.data.T_CONS.args[i], buffer);
          if (i < t.data.T_CONS.num_args - 1) {
            buffer = strcat(buffer, " * ");
          }
        }
        break;
      }
      break;
    }
  }

  return buffer;
}

// clang-format off
void print_type(Type t) {
  char *buf = malloc(sizeof(char) * 100);
  printf("%s\n", type_to_string(t, buf));
  free(buf);
}

Type *fn_ret_type(Type *fn_type) {
  if (fn_type->kind != T_FN) {
      // If it's not a function type, it's the return type itself
      return fn_type;
  }
  // Recursively check the 'to' field
  return fn_ret_type(fn_type->data.T_FN.to);
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
    /*
    if (env->type->kind == T_VARIANT) {
      Type *variant = env->type;
      for (int i = 0; i < variant->data.T_VARIANT.num_args; i++) {
        Type *variant_member = variant->data.T_VARIANT.args[i];
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
    */
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
  printf("%s : ", env->name);
  print_type(*env->type);
  printf("\n");
  if (env->next) {
    print_type_env(env->next);
  }
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
  fprintf(stderr, "Error: type or typeclass %s not found\n", id_chars);

  return NULL;
}

Type *get_type(TypeEnv *env, const char *name) {
  Type *_type = env_lookup(env, name);
  if (!_type) {
    _type = get_builtin_type(name);
    if (!_type) {
      return NULL;
    }
  }
  return _type;
}


void set_tuple_type(Type *type, Type *cons_args, int arity) {
  type->kind = T_CONS;
  type->data.T_CONS.name = "Tuple";
  type->data.T_CONS.args = cons_args;
  type->data.T_CONS.num_args = arity;
}

void set_list_type(Type *type, Type *el_type) {
  type->kind = T_CONS;
  type->data.T_CONS.name = "List";
  type->data.T_CONS.args = el_type;
  type->data.T_CONS.num_args = 1;
}
