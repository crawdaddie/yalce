// Helper function to print types (for debugging)
#include "parse.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool is_list_type(Type *type) {
  return type->kind == T_CONS && (strcmp(type->data.T_CONS.name, "List") == 0);
}

bool is_tuple_type(Type *type) {
  return type->kind == T_CONS && (strcmp(type->data.T_CONS.name, "Tuple") == 0);
}

void print_type(Type *type) {
  if (type == NULL) {
    printf("NULL");
    return;
  }

  switch (type->kind) {

  case T_INT:
    printf("Int");
    break;

  case T_NUM:
    printf("Double");
    break;

  case T_BOOL:
    printf("Bool");
    break;

  case T_STRING:
    printf("String");
    break;

  case T_VOID:
    printf("()");
    break;

  case T_VAR:
    printf("%s", type->data.T_VAR);
    break;

  case T_CONS:
    printf("%s", type->data.T_CONS.name);
    if (type->data.T_CONS.num_args > 0) {
      printf("(");
      for (int i = 0; i < type->data.T_CONS.num_args; i++) {
        if (i > 0)
          printf(", ");
        print_type(type->data.T_CONS.args[i]);
      }
      printf(")");
    }
    break;

  case T_FN:
    printf("(");
    print_type(type->data.T_FN.from);
    printf(" -> ");
    print_type(type->data.T_FN.to);
    printf(")");
    break;
  default:
    printf("Unknown");
    break;
  }
}

// Helper function to print type schemes (for debugging)
void print_type_scheme(TypeScheme *scheme) {
  if (scheme == NULL) {
    printf("NULL");
    return;
  }

  if (scheme->num_variables > 0) {
    printf("forall ");
    for (int i = 0; i < scheme->num_variables; i++) {
      if (i > 0)
        printf(", ");
      printf("%s", scheme->variables[i]);
    }
    printf(". ");
  }

  print_type(scheme->type);
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
    false;
  }
}

// Helper function to get the most general numeric type
Type *get_general_numeric_type(Type *t1, Type *t2) {
  if (t1->kind == T_NUM || t2->kind == T_NUM) {
    return &t_num;
  }
  return &t_int;
}

Type *builtin_type(Ast *id) {
  if (id->tag == AST_VOID) {
    return &t_void;
  }
  if (id->tag != AST_IDENTIFIER) {
    return NULL;
  }

  const char *id_chars = id->data.AST_IDENTIFIER.value;

  if (strcmp(id_chars, "int") == 0) {
    return &t_int;
  } else if (strcmp(id_chars, "double") == 0) {
    return &t_num;
  } else if (strcmp(id_chars, "bool") == 0) {
    return &t_bool;
  } else if (strcmp(id_chars, "string") == 0) {
    return &t_string;
  }

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
