#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "types/type.h"
void reset_type_var_counter();

typedef struct TICtx {
  TypeEnv *env;
  TypeConstraint *constraints;
  Ast *current_fn_ast;
  int scope;

} TICtx;

Type *infer(Ast *ast, TICtx *ctx);
Type *next_tvar();

void initialize_builtin_types();
#endif
