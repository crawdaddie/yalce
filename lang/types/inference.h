#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "../parse.h"
#include "type.h"

// TypeEnv represents a mapping from variable names to their types
// typedef struct TypeEnv {
//   const char *name;
//   Type *type;
//   int ref_count;
//   int is_fn_param;
//   int is_recursive_fn_ref;
//   int yield_boundary;
//   struct TypeEnv *next;
// } TypeEnv;

// Type *env_lookup(TypeEnv *env, const char *name);

typedef struct VarList {
  const char *var;
  struct VarList *next;
} VarList;

// represents a polymorphic type scheme
// eg: ∀α. α→α -> { vars: [α,] type: α -> α (fn type) }
// or ∀αβ. α→β→α -> { vars: [α,β] type: α -> β -> α (fn type) }
// if vars is NULL then we have a non-polymorphic type
typedef struct Scheme {
  VarList *vars;
  Type *type;
} Scheme;

// TypeScheme Env - maps names to type schemes
typedef struct TypeEnv {
  const char *name;
  Scheme scheme;
  struct TypeEnv *next;
} TypeEnv;

typedef struct Subst {
  const char *var;
  Type *type;
  struct Subst *next;
} Subst;

typedef struct TICtx {
  Subst *subst;
  TypeEnv *env;
  Ast *current_fn_ast;
  Type *yielded_type;
  int scope;
  int current_fn_scope;
  FILE *err_stream; // Replace const char *err
} TICtx;

typedef struct InferenceTree {
  const char *rule;
  const char *input;
  const char *output;
  int num_children;
  struct InferenceTree *children;
} InferenceTree;

typedef struct {
  int status;
  Subst *subst;
  Type *type;
  InferenceTree *tree;
} InferenceResult;
Type *infer(Ast *ast, TICtx *ctx);

void initialize_builtin_schemes();
void add_builtin(char *name, Type *t);

void print_builtin_types();

Scheme generalize(Type *type, TypeEnv *env);

Scheme *lookup_scheme(TypeEnv *env, const char *name);
Type *find_in_subst(Subst *subst, const char *name);
Subst *subst_extend(Subst *s, const char *key, Type *type);
Subst *compose_subst(Subst *s1, Subst *s2);
TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env);

TypeEnv *env_extend(TypeEnv *env, const char *name, VarList *names, Type *type);
#endif
