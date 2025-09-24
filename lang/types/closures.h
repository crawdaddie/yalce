#ifndef _LANG_TYPE_CLOSURES_H
#define _LANG_TYPE_CLOSURES_H
#include "types/inference.h"
#include "types/type.h"

Type *get_full_fn_type_of_closure(Ast *closure);

bool is_ref_to_closed_value(Ast *identifier);

void handle_closed_over_ref(Ast *ast, TypeEnv *ref, TICtx *ctx);

#endif
