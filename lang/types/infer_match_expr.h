#ifndef _LANG_TYPES_INFER_MATCH_H
#define _LANG_TYPES_INFER_MATCH_H
#include "parse.h"
#include "types/type.h"

Type *infer_match(Ast *ast, TypeEnv **env);

#endif
