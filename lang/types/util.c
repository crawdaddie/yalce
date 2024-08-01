// Helper function to print types (for debugging)
#include "types/util.h"
#include "parse.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_list_type(Type *type) {
  return type->kind == T_CONS && (strcmp(type->data.T_CONS.name, "List") == 0);
}

bool is_string_type(Type *type) {
  return type->kind == T_CONS &&
         (strcmp(type->data.T_CONS.name, "List") == 0) &&
         (type->data.T_CONS.args[0]->kind == T_CHAR);
}

bool is_tuple_type(Type *type) {
  return type->kind == T_CONS && (strcmp(type->data.T_CONS.name, "Tuple") == 0);
}

void print_type(Type *type) {

  if (type == NULL) {
    printf("NULL");
    return;
  }
  if (type->alias) {
    printf("%s", type->alias);
    return;
  }

  if (type->kind == T_MODULE) {
    printf("Module: \n");
    print_type_env(type->data.T_MODULE);
    return;
  }

  TypeSerBuf *b = create_type_ser_buffer(100);
  serialize_type(type, b);
  printf("%s", (char *)b->data);
  free(b->data);
  free(b);
}

void print_type_w_tc(Type *type) {
  print_type(type);
  if (type->num_implements == 0) {
    return;
  }
  printf(" implements : [");
  for (int i = 0; i < type->num_implements; i++) {
    TypeClass *tc = type->implements[i];
    printf("%s, ", tc->name);
  }
  printf("]");
}

// Helper function to print the type environment (for debugging)
void print_type_env(TypeEnv *env) {
  while (env != NULL) {
    printf("%s : ", env->name);
    print_type(env->type);
    printf("\n");
    env = env->next;
  }
}

// Helper functions
bool is_numeric_type(Type *type) {
  return type->kind == T_INT || type->kind == T_NUM;
}

int fn_type_args_len(Type *fn_type) {
  Type *t = fn_type;
  int fn_len = 0;

  while (t->kind == T_FN) {
    Type *from = t->data.T_FN.from;
    t = t->data.T_FN.to;
    fn_len++;
  }
  return fn_len;
}

bool is_type_variable(Type *type) { return type->kind == T_VAR; }
bool is_generic(Type *type) {
  switch (type->kind) {
  case T_VAR: {
    return true;
  }

  case T_CONS: {
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      if (is_generic(type->data.T_CONS.args[i])) {
        return true;
      }
    }
    return false;
  }

  case T_FN: {
    if (is_generic(type->data.T_FN.from)) {
      return true;
    }

    if (type->data.T_FN.to->kind != T_FN) {
      // return type of function (doesn't matter for genericity)
      return false;
    }

    return is_generic(type->data.T_FN.to);
  }

  default:
    return false;
  }
}

// Helper function to get the most general numeric type
Type *get_general_numeric_type(Type *t1, Type *t2) {
  if (t1->kind == T_NUM || t2->kind == T_NUM) {
    return &t_num;
  }
  return &t_int;
}


Type *get_type(TypeEnv *env, Ast *id) {
  if (id->tag == AST_VOID) {
    return &t_void;
  }
  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  const char *id_chars = id->data.AST_IDENTIFIER.value;
  Type *named_type = env_lookup(env, id_chars);
  if (named_type) {
    return named_type;
  }

  if (strcmp(id_chars, TYPE_NAME_INT) == 0) {
    return &t_int;
  } else if (strcmp(id_chars, TYPE_NAME_DOUBLE) == 0) {
    return &t_num;
  } else if (strcmp(id_chars, TYPE_NAME_BOOL) == 0) {
    return &t_bool;
  } else if (strcmp(id_chars, TYPE_NAME_STRING ) == 0) {
    return &t_string;
  } else if (strcmp(id_chars, TYPE_NAME_PTR) == 0) {
    return &t_ptr;
  }
  fprintf(stderr, "Error: type or typeclass %s not found\n", id_chars);

  return NULL;
}

bool types_equal(Type *t1, Type *t2) {
  if (t1 == t2) {
    return true;
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
    return strcmp(t1->data.T_VAR, t2->data.T_VAR) == 0;
  }

  case T_CONS: {
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
  }
  return false;
}

// Deep copy implementation (simplified)
Type *deep_copy_type(const Type *original) {
  if (!is_generic(original)) {
    return original;
  }
  Type *copy = malloc(sizeof(Type));
  copy->kind = original->kind;
  copy->alias = original->alias;
  for (int i = 0; i < original->num_implements; i++) {
    add_typeclass_impl(copy, original->implements[i]);
  }

  switch (original->kind) {
  case T_VAR:
    copy->data.T_VAR = strdup(original->data.T_VAR);
    break;
  case T_CONS:
    // Deep copy of name and args
    copy->data.T_CONS.name = strdup(original->data.T_CONS.name);
    copy->data.T_CONS.num_args = original->data.T_CONS.num_args;
    copy->data.T_CONS.args =
        malloc(sizeof(Type *) * copy->data.T_CONS.num_args);
    for (int i = 0; i < copy->data.T_CONS.num_args; i++) {
      copy->data.T_CONS.args[i] = deep_copy_type(original->data.T_CONS.args[i]);
    }
    break;
  case T_FN:
    copy->data.T_FN.from = deep_copy_type(original->data.T_FN.from);
    copy->data.T_FN.to = deep_copy_type(original->data.T_FN.to);
    break;
  }
  return copy;
}

// Deep free a type var
void free_type(const Type *type) {
  switch (type->kind) {
  case T_VAR:
    // free((void *)type->data.T_VAR);
    break;
  case T_CONS:
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      free_type(type->data.T_CONS.args[i]);
    }
    free(type->data.T_CONS.args);
    break;
  case T_FN:
    free_type(type->data.T_FN.from);
    free_type(type->data.T_FN.to);
    break;
  }
  free((void *)type);
}

TypeSerBuf *create_type_ser_buffer(size_t initial_capacity) {
  TypeSerBuf *buf = malloc(sizeof(TypeSerBuf));
  buf->data = malloc(initial_capacity);
  buf->size = 0;
  buf->capacity = initial_capacity;
  return buf;
}

static void buffer_write(TypeSerBuf *buf, const void *data, size_t size) {
  if (buf->size + size > buf->capacity) {
    buf->capacity = (buf->size + size) * 2;
    buf->data = realloc(buf->data, buf->capacity);
  }
  memcpy(buf->data + buf->size, data, size);
  buf->size += size;
}

void serialize_type(Type *type, TypeSerBuf *buf) {
  if (type == NULL) {
    uint8_t kind = 0xFF; // Special value for NULL
    buffer_write(buf, &kind, sizeof(uint8_t));
    return;
  }

  // buffer_write(buf, &type->kind, sizeof(uint8_t));

  switch (type->kind) {
  case T_VAR: {
    size_t len = strlen(type->data.T_VAR);
    buffer_write(buf, type->data.T_VAR, len);
    break;
  }
  case T_CONS: {
    if (strcmp(type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0) {

      buffer_write(buf, "(", 1);
      for (int i = 0; i < type->data.T_CONS.num_args; i++) {
        serialize_type(type->data.T_CONS.args[i], buf);
        if (i < type->data.T_CONS.num_args - 1) {
          buffer_write(buf, " * ", 3);
        }
      }
      buffer_write(buf, ")", 1);
      break;
    }

    if (is_string_type(type)) {
      buffer_write(buf, TYPE_NAME_STRING, 6);
      break;
    }

    if (strcmp(type->data.T_CONS.name, TYPE_NAME_LIST) == 0) {

      buffer_write(buf, "[", 1);
      serialize_type(type->data.T_CONS.args[0], buf);
      buffer_write(buf, "]", 1);
      break;
    }
    buffer_write(buf, "cons(", 5);
    buffer_write(buf, type->data.T_CONS.name, strlen(type->data.T_CONS.name));
    buffer_write(buf, ", ", 2);
    char int_str[32];
    int length =
        snprintf(int_str, sizeof(int_str), "%d", type->data.T_CONS.num_args);
    buffer_write(buf, int_str, length);
    buffer_write(buf, ", ", 2);
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      serialize_type(type->data.T_CONS.args[i], buf);
      if (i < type->data.T_CONS.num_args - 1) {
        buffer_write(buf, ", ", 2);
      }
    }
    buffer_write(buf, ")", 1);
    break;
  }
  case T_FN:
    buffer_write(buf, "(", 1);
    serialize_type(type->data.T_FN.from, buf);
    buffer_write(buf, " -> ", 4);
    serialize_type(type->data.T_FN.to, buf);
    buffer_write(buf, ")", 1);
    break;

  case T_INT:
    buffer_write(buf,TYPE_NAME_INT, 3);
    break;

  case T_NUM:
    buffer_write(buf,TYPE_NAME_DOUBLE, 6);
    break;

  case T_BOOL:

    buffer_write(buf, TYPE_NAME_BOOL, 4);
    break;

  case T_STRING:
    buffer_write(buf, TYPE_NAME_STRING, 6);
    break;

  case T_VOID:
    buffer_write(buf, "()", 2);
    break;

  case T_CHAR:
    buffer_write(buf, TYPE_NAME_CHAR, 4);
    break;

  default:
    // No additional data for other types
    break;
  }
}

static void free_buffer(TypeSerBuf *buf) {
  free(buf->data);
  free(buf);
}
