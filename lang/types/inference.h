#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "type.h"

void reset_type_var_counter();
int infer(Ast *, TypeEnv **);
#endif
