#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "../arena_allocator.h"
#include "../parse.h"
#include "./type.h"

DECLARE_ARENA_ALLOCATOR_DEFAULT(t);
void reset_type_var_counter();

typedef struct Subst {
  const char *var;
  Type *type;
  struct Subst *next;
} Subst;

typedef struct Constraint {
  Type *var;  // Variable type (e.g., "t0")
  Type *type; // Required type (e.g., Int or Double)
  struct Constraint *next;
} Constraint;
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

// TypeEnv represents a mapping from variable names to their types
typedef struct TypeEnv {
  const char *name;
  Type *type;
  binding_md md;
  int ref_count;

  struct TypeEnv *next;
} TypeEnv;

typedef struct TICtx {
  Subst *subst;
  TypeEnv *env;
  Ast *current_fn_ast;
  Constraint *constraints;
  Type *yielded_type;
  int scope;
  int current_fn_base_scope;
  void *type_decl_ctx;
  custom_binops_t *custom_binops;
  FILE *err_stream; // Replace const char *err
} TICtx;

Type *infer(Ast *ast, TICtx *ctx);

typedef struct VarList {
  const char *var;
  struct VarList *next;
  TypeClass *implements;
} VarList;

// typedef struct Scheme {
//   VarList *vars;
//   Type *type;
// } Scheme;

Type *infer(Ast *ast, TICtx *ctx);
Type *generalize(Type *t, TICtx *ctx);
Type *instantiate(Type *sch, TICtx *ctx);
Type *instantiate_type_in_env(Type *sch, TypeEnv *env);
Type *env_lookup(TypeEnv *env, const char *name);
TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
Type *extract_member_from_sum_type(Type *cons, Ast *id);
void *type_error(Ast *ast, const char *fmt, ...);

Constraint *merge_constraints(Constraint *list1, Constraint *list2);
Subst *solve_constraints(Constraint *constraints);
Subst *compose_subst(Subst *s1, Subst *s2);
Type *apply_substitution(Subst *subst, Type *t);

TypeEnv *apply_subst_env(Subst *subst, TypeEnv *env);

int unify(Type *t1, Type *t2, TICtx *unify_res);

void print_constraints(Constraint *constraints);

void print_subst(Subst *subst);

int bind_type_in_ctx(Ast *binding, Type *type, binding_md binding_type,
                     TICtx *ctx);
TypeEnv *lookup_type_ref(TypeEnv *env, const char *name);

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst);
void add_constraint(TICtx *result, Type *var, Type *type);

Type *resolve_type_in_env(Type *r, TypeEnv *env);

Type *resolve_tc_rank(Type *type);
Type *resolve_tc_rank_in_env(Type *type, TypeEnv *env);

Type *find_in_subst(Subst *subst, const char *name);

#endif
