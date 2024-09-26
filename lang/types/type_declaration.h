#ifndef _LANG_TYPE_DECL_H
#define _LANG_TYPE_DECL_H
#include "../parse.h"
#include "type.h"
Type *type_declaration(Ast *ast, TypeEnv **env);

Type *compute_type_expression(Ast *expr, TypeEnv *env);
#endif
