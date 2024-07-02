#include "types/type.h"
#include <stdlib.h>
#include <string.h>

Type t_int = {T_INT};
Type t_num = {T_NUM};
Type t_string = {T_STRING};
Type t_bool = {T_BOOL};
Type t_void = {T_VOID};

// add a new typescheme to an env
TypeEnv extend_env(TypeEnv env, const char *name, TypeScheme *scheme) {
  TypeEnvNode *new_node = malloc(sizeof(TypeEnvNode));
  new_node->name = strdup(name);
  new_node->scheme = scheme;
  new_node->next = env;
  return new_node;
}

// looks up a name in a TypeEnv env
TypeScheme *lookup_env(TypeEnv env, const char *name) {
  while (env != NULL) {
    if (strcmp(env->name, name) == 0) {
      return env->scheme;
    }
    env = env->next;
  }
  return NULL;
}
