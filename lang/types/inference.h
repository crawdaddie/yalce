#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "../parse.h"
#include "type.h"
void reset_type_var_counter();

Substitution *substitutions_extend(Substitution *subst, Type *t1, Type *t2);

Type *apply_substitution(Substitution *subst, Type *t);
void print_subst(Substitution *c);

Type *infer(Ast *ast, TICtx *ctx);
Type *next_tvar();

void initialize_builtin_types();
void add_builtin(char *name, Type *t);

void print_builtin_types();

Type *env_lookup(TypeEnv *env, const char *name);

TypeEnv *env_lookup_ref(TypeEnv *env, const char *name);

Type *solve_program_constraints(Ast *prog, TICtx *ctx);

TypeConstraint *constraints_extend(TypeConstraint *constraints, Type *t1,
                                   Type *t2);

Substitution *solve_constraints(TypeConstraint *constraints);

bool is_loop_of_iterable(Ast *let);

Type *get_full_fn_type_of_closure(Ast *closure);

Type *coroutine_constructor_type_from_fn_type(Type *fn_type
                                              // , Ast *ast
);

typedef struct VarList {
  const char *var;
  struct VarList *next;
  TypeClass *implements;
} VarList;

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


typedef struct Subst {
  const char *var;
  Type *type;
  struct Subst *next;
} Subst;

typedef struct Constraint {
} Constraint;

Scheme generalize(Type *t, TICtx *ctx);
Type *instantiate(Scheme *sch, TICtx *ctx);

#endif
