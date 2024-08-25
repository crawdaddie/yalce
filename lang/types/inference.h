#ifndef _LANG_TYPE_INFERENCE_H
#define _LANG_TYPE_INFERENCE_H
#include "parse.h"
#include "type.h"

int infer(Ast *, TypeEnv **);
#endif
