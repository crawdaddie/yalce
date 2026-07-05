#include "./type_ser.h"
#include "./type.h"
#include <string.h>

void print_type_env_stream(TypeEnv *env, FILE *stream);
static char *type_name_mapping[] = {
    [T_INT] = TYPE_NAME_INT,    [T_UINT64] = TYPE_NAME_UINT64,
    [T_NUM] = TYPE_NAME_DOUBLE, [T_BOOL] = TYPE_NAME_BOOL,
    [T_VOID] = TYPE_NAME_VOID,  [T_CHAR] = TYPE_NAME_CHAR,
};

char *type_to_string_dynamic(Type *t) {
  char *buffer = NULL;
  size_t size = 0;

  FILE *stream = open_memstream(&buffer, &size);
  if (!stream) {
    return NULL;
  }

  print_type_to_stream(t, stream);
  fclose(stream); // This finalizes and null-terminates the buffer

  return buffer; // Caller must free() this
}

char *type_to_string(Type *t, char *buffer) {
  return type_to_string_dynamic(t);
}

void print_tc_list_to_stream(Type *t, FILE *stream) {
  if (t->implements == NULL) {
    return;
  }

  fprintf(stream, " with ");
  for (TypeClass *i = t->implements; i; i = i->next) {
    if (i->module) {
      print_type_to_stream(i->module, stream);
    } else {
      fprintf(stream, "%s ", i->name);
      if (i->params) {
        fprintf(stream, "<");
        for (TypeList *p = i->params; p; p = p->next) {
          print_type_to_stream(p->type, stream);
          if (p->next) {
            fprintf(stream, ", ");
          }
        }
        fprintf(stream, ">");
      }
    }
    fprintf(stream, ",");
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

  case T_CONS:
  case T_SUM: {

    if (is_string_type(t)) {
      fprintf(stream, "String");
      break;
    }

    if (is_list_type(t)) {
      print_type_to_stream(type_of_list(t), stream);
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
          if (t->data.T_CONS.names != NULL) {
            fprintf(stream, "%s: ", t->data.T_CONS.names[i]);
          }
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
      if (t->data.T_CONS.args[0]->kind == T_CONS) {
        print_type_to_stream(t->data.T_CONS.args[0]->data.T_CONS.args[0],
                             stream);
      } else {
        print_type_to_stream(t->data.T_CONS.args[0], stream);
      }
      break;
    }

    if (t->kind == T_CONS && CHARS_EQ(t->data.T_CONS.name, TYPE_NAME_SOME)) {
      fprintf(stream, "Option of ");

      if (t->data.T_CONS.args[0]->kind == T_CONS) {
        print_type_to_stream(t->data.T_CONS.args[0]->data.T_CONS.args[0],
                             stream);
      } else {
        print_type_to_stream(t->data.T_CONS.args[0], stream);
      }
      break;
    }

    if (is_sum_type(t)) {
      fprintf(stream, "%s { ", t->data.T_CONS.name);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        print_type_to_stream(t->data.T_CONS.args[i], stream);
        if (i < t->data.T_CONS.num_args - 1) {
          fprintf(stream, " | ");
        }
      }

      fprintf(stream, " }");
      break;
    }
    if (t->alias) {
      fprintf(stream, "%s", t->alias);
      // print_tc_list_to_stream(t, stream);
      // print_tc_list_to_stream(t, stream);
      // u
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
    int vid = t->data.T_VAR.id;

    if (vid >= 0 && vid < 65) {
      // fprintf(stream, "%c", (char)(vid + 65));
      fprintf(stream, "`%d", vid);
    } else if (t->data.T_VAR.name) {
      fprintf(stream, "%s", t->data.T_VAR.name);
    } else {
      fprintf(stream, "`%d", vid);
    }

    print_tc_list_to_stream(t, stream);
    break;
  }

  case T_RECURSIVE_REF: {
    fprintf(stream, "%s", t->data.T_RECURSIVE_REF.name);
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
  case T_MODULE: {
    fprintf(stream, "%s", TYPE_NAME_MODULE);
    if (t->data.T_MODULE.size > 0) {
      fprintf(stream, " of \n");
    }
    for (TypeEnv *te = t->data.T_MODULE.env; te; te = te->next) {
      fprintf(stream, "%s: ", te->name);
      if (te->scheme_vars) {
        fprintf(stream, "∀ ");
        for (TypeList *v = te->scheme_vars; v; v = v->next) {
          print_type_to_stream(v->type, stream);
          fprintf(stream, ", ");
        }
        fprintf(stream, ": ");
      }
      print_type_to_stream(te->type, stream);
      fprintf(stream, "\n");
    }
    break;
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

void print_type_env_stream(TypeEnv *env, FILE *stream) {
  if (!env) {
    return;
  }
  fprintf(stream, "%s : ", env->name);
  if (env->scheme_vars) {
    fprintf(stream, "∀ ");
    for (TypeList *v = env->scheme_vars; v; v = v->next) {
      Type *n = v->type;
      print_type_to_stream(n, stream);
      fprintf(stream, ", ");
    }

    fprintf(stream, ": ");
  }

  print_type_to_stream(env->type, stream);
}

void print_type_env(TypeEnv *env) {
  for (TypeEnv *e = env; e; e = e->next) {
    print_type_env_stream(e, stdout);
    if (e->next) {
      fprintf(stdout, "\n");
    }
  }
}
