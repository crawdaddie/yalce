#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "../arena_allocator.h"
#include "../parse.h"
#include "./type.h"

DECLARE_ARENA_ALLOCATOR_DEFAULT(t);
void reset_type_var_counter();
void mark_type_var_counter_floor();

typedef struct LambdaScope {
  Ast *fn_ast;
  int base_scope;
  struct LambdaScope *parent;
} LambdaScope;

typedef struct TICtx {
  Subst *subst;
  TypeEnv *env;

  Ast *current_fn_ast;
  LambdaScope *current_scope;

  Constraint *constraints;
  Predicate *predicates; // accumulated trait obligations
  Type *yielded_type;
  int scope;
  int current_fn_base_scope;
  void *type_decl_ctx;
  custom_binops_t *custom_binops;
  FILE *err_stream; // Replace const char *err
} TICtx;

typedef struct {
  Subst *subst;
} Solution;

Type *infer(Ast *ast, TICtx *ctx);
int infer_solve(TICtx *ctx, Solution *sol);

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

// New binding-based polymorphism (HM core)
void generalize_env(TypeEnv *entry, TypeEnv *env);
Type *instantiate_env(TypeEnv *entry, TICtx *ctx);

// Backward-compatible T_SCHEME wrappers (transitionary)
Type *generalize(Type *t, TICtx *ctx);
Type *instantiate(Type *sch, TICtx *ctx);
Type *instantiate_type_in_env(Type *sch, TypeEnv *env);
Type *env_lookup(TypeEnv *env, const char *name);
TypeEnv *env_extend(TypeEnv *env, const char *name, Type *type);
TypeEnv *env_extend_with_preds(TypeEnv *env, const char *name, Type *type,
                               Predicate *preds);

TypeList *free_vars_type(TypeList *acc, Type *t);
TypeList *free_vars_env(TypeList *acc, TypeEnv *env);

Type *extract_member_from_sum_type(Type *cons, Ast *id);
Type *extract_member_from_sum_type_idx(Type *cons, Ast *id, int *idx);
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

bool is_list_cons_operator(Ast *ast);

void apply_substitution_to_lambda_body(Ast *ast, Subst *subst);
void add_constraint(TICtx *result, Type *var, Type *type);

Type *resolve_type_in_env(Type *r, TypeEnv *env);

Type *resolve_tc_rank(Type *type);
Type *resolve_tc_rank_in_env(Type *type, TypeEnv *env);

Type *find_in_subst(Subst *subst, int var_id);

bool is_constant_expr(Ast *expr, TICtx *ctx);

// Predicate helpers
Predicate *predicate_append(Predicate *list, TypeClass *trait, Type *type);
Predicate *predicate_append_applied(Predicate *list, TypeClass *trait,
                                    Type *type, TypeList *params);
Predicate *predicate_append_comparable(Predicate *list, TypeClass *trait,
                                       Type *witness, Type **args);
Predicate *predicate_apply_subst(Subst *subst, Predicate *preds);
Predicate *predicate_duplicate(Predicate *preds);
int resolve_predicates(Subst **subst, Predicate *preds, FILE *err_stream);

void print_predicates(Predicate *predicates);

int bind_pattern(Ast *pattern, Type *value_type, TICtx *ctx);
#endif
