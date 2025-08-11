#ifndef _LANG_TYPE_DECL_H
#define _LANG_TYPE_DECL_H
#include "../parse.h"
#include "type.h"
Type *type_declaration(Ast *ast, TypeEnv **env);

typedef struct {
  TypeEnv *env;
} TDCtx;
Type *compute_type_expression(Ast *expr, TDCtx *ctx);
#endif
