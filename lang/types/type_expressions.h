#ifndef _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#define _LANG_TYPE_INFERENCE_TYPE_EXPRS_H
#include "../parse.h"
#include "./inference.h"

Type *compute_type_expression(Ast *expr, TICtx *ctx);
Type *infer_type_declaration(Ast *ast, TICtx *ctx);

void compute_lambda_param_types(AstList *annotations, size_t len, Type **out,
                                TICtx *ctx);

// Like compute_lambda_param_types, but does NOT reset current_type_var_env
// to NULL first. Used by parametrized modules so the tvars introduced while
// computing param annotations remain visible to body type-expression
// resolution, letting the body share the module's generic type variables.
void compute_module_param_types(AstList *annotations, size_t len, Type **out,
                                TICtx *ctx);

// Seed/reset the type-variable name environment used by
// compute_type_expression_inner when resolving bare type-variable names.
// Parametrized-module inference calls this to make the module's generic
// tvars resolvable by name across the body's type annotations, which would
// otherwise each run with a fresh (NULL) type-var env.
void set_type_var_env(TypeEnv *env);
TypeEnv *get_type_var_env(void);

Type *compute_fn_type(Ast *expr, TICtx *ctx);
#endif
