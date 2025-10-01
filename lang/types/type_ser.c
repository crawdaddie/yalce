#include "./type_ser.h"
#include "./type.h"
#include <string.h>

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

    if (is_string_type(t)) {
      buffer = strcat(buffer, "String");
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
    if (is_sum_type(t) && t->data.T_CONS.args[0]->kind == T_CONS &&
        CHARS_EQ(t->data.T_CONS.args[0]->data.T_CONS.name, "Some")) {

      buffer = strncat(buffer, "Option of ", 10);
      buffer =
          type_to_string(t->data.T_CONS.args[0]->data.T_CONS.args[0], buffer);
      break;
    }

    if (t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, "Some")) {
      buffer = strncat(buffer, "Option of ", 10);
      buffer =
          type_to_string(t->data.T_CONS.args[0]->data.T_CONS.args[0], buffer);
      break;
    }

    if (is_sum_type(t)) {

      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        buffer = type_to_string(t->data.T_CONS.args[i], buffer);
        if (i < t->data.T_CONS.num_args - 1) {
          buffer = strncat(buffer, " | ", 3);
        }
      }
      break;
    }

    if (t->alias) {
      buffer = strcat(buffer, t->alias);
      // buffer = tc_list_to_string(t, buffer);
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
        if (t->data.T_CONS.names) {
          buffer = strcat(buffer, t->data.T_CONS.names[i]);
          buffer = strcat(buffer, ": ");
        }
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
    if (is_closure(fn)) {
      buffer = strcat(buffer, "[closure over ");
      buffer = type_to_string(fn->closure_meta, buffer);
      buffer = strcat(buffer, "] ");
    }

    buffer = type_to_string(fn->data.T_FN.from, buffer);
    buffer = strncat(buffer, " -> ", 4);

    buffer = type_to_string(fn->data.T_FN.to, buffer);
    buffer = strcat(buffer, ")");

    break;
  }

  case T_SCHEME: {

    strncat(buffer, "∀ ", 2);
    for (TypeList *v = t->data.T_SCHEME.vars; v; v = v->next) {
      Type *n = v->type;
      buffer = type_to_string(n, buffer);
      strncat(buffer, ", ", 2);
    }

    strncat(buffer, ": ", 2);
    buffer = type_to_string(t->data.T_SCHEME.type, buffer);
  }
  }

  return buffer;
}

void print_tc_list_to_stream(Type *t, FILE *stream) {
  if (t->implements == NULL) {
    return;
  }

  fprintf(stream, " with ");
  for (TypeClass *i = t->implements; i; i = i->next) {
    fprintf(stream, "%s, ", i->name);
  }
}

void print_type_to_stream(Type *t, FILE *stream) {
  if (t == NULL) {
    fprintf(stream, "null");
    return;
  }

  // if (t->alias != NULL) {
  //   fprintf(stream, "%s", t->alias);
  //   return;
  // }

  switch (t->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_BOOL:
  case T_VOID:
  case T_CHAR: {
    char *m = type_name_mapping[t->kind];
    fprintf(stream, "%s", m);
    break;
  }
  case T_EMPTY_LIST: {
    fprintf(stream, "[]");
    break;
  }

  case T_TYPECLASS_RESOLVE: {
    fprintf(stream, "tc resolve %s [ ", t->data.T_CONS.name);

    int len = t->data.T_CONS.num_args;
    for (int i = 0; i < len - 1; i++) {
      print_type_to_stream(t->data.T_CONS.args[i], stream);
    }

    fprintf(stream, " : ");
    print_type_to_stream(t->data.T_CONS.args[len - 1], stream);

    fprintf(stream, "]");
    break;
  }
  case T_CONS: {

    if (is_string_type(t)) {
      fprintf(stream, "String");
      break;
    }

    if (is_list_type(t)) {
      print_type_to_stream(t->data.T_CONS.args[0], stream);
      fprintf(stream, "[]");
      break;
    }
    if (strcmp(t->data.T_CONS.name, "Module") == 0) {

      // print_type_to_stream(t->data.T_CONS.args[0], stream);
      // fprintf(stream, "[]");

      fprintf(stream, "%s", t->data.T_CONS.name);
      if (t->data.T_CONS.num_args > 0) {
        fprintf(stream, " of \n");
        for (int i = 0; i < t->data.T_CONS.num_args; i++) {

          fprintf(stream, "%s: ", t->data.T_CONS.names[i]);
          print_type_to_stream(t->data.T_CONS.args[i], stream);
          fprintf(stream, "\n");
        }
      }
      break;
    }

    if (is_tuple_type(t)) {
      fprintf(stream, "(");
      int is_named = t->data.T_CONS.names != NULL;
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        if (is_named) {
          fprintf(stream, "%s: ", t->data.T_CONS.names[i]);
        }
        print_type_to_stream(t->data.T_CONS.args[i], stream);
        if (i < t->data.T_CONS.num_args - 1) {
          fprintf(stream, " * ");
        }
      }

      fprintf(stream, " )");
      break;
    }

    if (is_sum_type(t) && t->data.T_CONS.args[0]->kind == T_CONS &&
        CHARS_EQ(t->data.T_CONS.args[0]->data.T_CONS.name, "Some")) {

      fprintf(stream, "Option of ");
      print_type_to_stream(t->data.T_CONS.args[0]->data.T_CONS.args[0], stream);
      break;
    }

    if (t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, "Some")) {
      fprintf(stream, "Option of ");
      print_type_to_stream(t->data.T_CONS.args[0]->data.T_CONS.args[0], stream);
      break;
    }

    if (is_sum_type(t)) {
      fprintf(stream, "%s ", t->data.T_CONS.name);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        print_type_to_stream(t->data.T_CONS.args[i], stream);
        if (i < t->data.T_CONS.num_args - 1) {
          fprintf(stream, " | ");
        }
      }
      break;
    }
    if (t->alias) {
      fprintf(stream, "%s", t->alias);
      // print_tc_list_to_stream(t, stream);
      // print_tc_list_to_stream(t, stream);
      break;
    }

    fprintf(stream, "%s", t->data.T_CONS.name);
    if (t->data.T_CONS.num_args > 0) {
      fprintf(stream, " of ");
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        if (t->data.T_CONS.names) {
          fprintf(stream, "%s: ", t->data.T_CONS.names[i]);
        }
        print_type_to_stream(t->data.T_CONS.args[i], stream);
        if (i < t->data.T_CONS.num_args - 1) {
          fprintf(stream, ", ");
        }
      }
    }
    print_tc_list_to_stream(t, stream);
    break;
  }
  case T_VAR: {
    uint64_t vname = (uint64_t)t->data.T_VAR;
    if (vname < 65) {
      vname += 65;
      fprintf(stream, "%c", (char)vname);
    } else {
      fprintf(stream, "%s", t->data.T_VAR);
    }

    print_tc_list_to_stream(t, stream);
    break;
  }

  case T_FN: {
    Type *fn = t;

    fprintf(stream, "(");

    if (is_closure(fn)) {
      fprintf(stream, "[closure over ");
      print_type_to_stream(fn->closure_meta, stream);
      fprintf(stream, "] ");
    }

    print_type_to_stream(fn->data.T_FN.from, stream);

    fprintf(stream, " -> ");
    print_type_to_stream(fn->data.T_FN.to, stream);
    fprintf(stream, ")");
    break;
  }
  case T_SCHEME: {

    fprintf(stream, "∀ ");
    for (TypeList *v = t->data.T_SCHEME.vars; v; v = v->next) {
      Type *n = v->type;
      print_type_to_stream(n, stream);
      fprintf(stream, ", ");
    }

    fprintf(stream, ": ");
    print_type_to_stream(t->data.T_SCHEME.type, stream);
  }
  }
}

// Updated print functions that use the stream-based approach
void print_type(Type *t) {
  if (!t) {
    printf("null\n");
    return;
  }

  // if (t->alias) {
  //   printf("%s\n", t->alias);
  //   return;
  // }

  fflush(stdout);
  print_type_to_stream(t, stdout);
  fflush(stdout);
  printf("\n");
}

void print_type_err(Type *t) {
  if (!t) {
    fprintf(stderr, "null\n");
    return;
  }

  print_type_to_stream(t, stderr);
  fprintf(stderr, "\n");
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
