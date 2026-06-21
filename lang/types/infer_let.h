#ifndef _LANG_TYPES_INFER_LET_H
#define _LANG_TYPES_INFER_LET_H
#include "inference.h"
#include "type.h"
Type *infer_let_expr(Ast *ast, TICtx *ctx);

void mark_generalizable_slice(TypeEnv *slice_head, TypeEnv *boundary);

void finalize_env_slice(TypeEnv *slice_head, TypeEnv *boundary, Subst *subst);
#endif
