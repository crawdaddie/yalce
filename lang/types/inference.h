#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "types/type.h"
void reset_type_var_counter();
Type *infer(Ast *ast, TypeEnv **env);
Type *next_tvar();
#endif
