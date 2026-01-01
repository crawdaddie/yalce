#ifndef _LANG_TYPE_INFER_LAMBDA_H
#define _LANG_TYPE_INFER_LAMBDA_H
#include "../parse.h"
#include "./inference.h"
Type *infer_lambda(Ast *ast, TICtx *ctx);

Type *create_coroutine_inst(Type *ret_type);

Type *lower_recursive_cons_ref(Type *t, TypeEnv *env);

Type *lower_recursive_ref(Type *t, TypeEnv *env);

bool is_recursive_ref_container(Type *t);
#endif
