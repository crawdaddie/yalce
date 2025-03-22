#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "../parse.h"
#include "type.h"
void reset_type_var_counter();

typedef struct TICtx {
  TypeEnv *env;
  TypeConstraint *constraints;
  Ast *current_fn_ast;
  Type *yielded_type;
  int scope;
  const char *err;
  FILE *err_stream; // Replace const char *err
} TICtx;

// Substitution map for type variables
typedef struct Substitution {
  Type *from; // Type variable
  Type *to;   // Replacement type
  struct Substitution *next;
} Substitution;

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

void print_constraints(TypeConstraint *c);
#endif
