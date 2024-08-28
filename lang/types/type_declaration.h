#ifndef _LANG_TYPE_DECL_H
#define _LANG_TYPE_DECL_H
#include "parse.h"
#include "types/type.h"
Type *type_declaration(Ast *ast, TypeEnv **env);
#endif
