#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H

#include "parse.h"

enum TypeKind {
  /* Type Operator */
  T_INT,
  T_NUM,
  T_STRING,
  T_BOOL,
  T_VOID,
  T_FN,
  T_PAIR,
  T_TUPLE,
  T_LIST,
  T_CONS,
  /* Type Variable  */
  T_VAR,
};

typedef struct Type {
  enum TypeKind kind;
  union {
    char *T_VAR; // for TYPE_VARIABLE
    struct {
      char *name;
      struct Type **args;
      int num_args;
    } T_CONS; // for TYPE_CONSTRUCTOR
    //
    struct {
      struct Type *from;
      struct Type *to;
    } T_FN; // for TYPE_FUNCTION
  } data;

} Type;

// TypeScheme structure
typedef struct TypeScheme {
  char **variables;
  int num_variables;
  Type *type;
} TypeScheme;

// TypeEnv structure
typedef struct TypeEnvNode {
  char *name;
  TypeScheme *scheme;
  struct TypeEnvNode *next;
} TypeEnvNode;

typedef TypeEnvNode *TypeEnv;

// Substitution structure
typedef struct Substitution {
  char *var_name;
  Type *type;
  struct Substitution *next;
} Substitution;

Type *infer_ast(TypeEnv env, Ast *ast);
void print_type(Type *type);

Type *create_function_type(Type *from, Type *to);

TypeEnv extend_env(TypeEnv env, const char *name, TypeScheme *scheme);

extern Type t_int;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;

void reset_type_var_counter();

#endif
