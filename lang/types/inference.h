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
typedef struct {
  enum BindingType {
    BT_VAR,
    BT_RECURSIVE_REF,
    BT_FN_PARAM,
  } type;

  union {
    struct {
      int scope;
      int yield_boundary_scope;
    } VAR;

    struct {
      int scope;
    } RECURSIVE_REF;

    struct {
      int scope;
    } FN_PARAM;

  } data;
} binding_md;

typedef struct TypeEnv {
  const char *name;
  Scheme scheme;
  binding_md md;
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
  Constraint *constraints;
  Type *yielded_type;
  int scope;
  int current_fn_base_scope;
  void *type_decl_ctx;
  FILE *err_stream; // Replace const char *err
} TICtx;

Type *infer(Ast *ast, TICtx *ctx);

void initialize_builtin_schemes();
void add_builtin(char *name, Type *t);

void print_builtin_types();

VarList *free_vars_type(Type *t);
Scheme generalize(Type *type, TypeEnv *env);
Type *instantiate(Scheme *scheme, TICtx *ctx);

Type *instantiate_with_args(Scheme *scheme, Ast *args, TICtx *ctx);

bool is_index_access_ast(Ast *application, Type *arg_type, Type *cons_type);

Scheme *lookup_scheme(TypeEnv *env, const char *name);
Type *find_in_subst(Subst *subst, const char *name);
Subst *subst_extend(Subst *s, const char *key, Type *type);
Subst *compose_subst(Subst *s1, Subst *s2);
TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env);

TypeEnv *env_extend(TypeEnv *env, const char *name, VarList *names, Type *type);

Type *apply_substitution(Subst *subst, Type *t);
void *type_error(TICtx *ctx, Ast *node, const char *fmt, ...);

void print_subst(Subst *subst);

void print_typescheme(Scheme scheme);
void print_type_env(TypeEnv *env);

bool varlist_contains(VarList *vs, const char *name);

VarList *varlist_add(VarList *vars, const char *v);

Type *env_lookup(TypeEnv *env, const char *name);

#endif
