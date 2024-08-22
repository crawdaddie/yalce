#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "types/type.h"
Type *infer_ast(TypeEnv **env, Ast *ast);
void reset_type_var_counter();

Type *next_tvar();

#endif
