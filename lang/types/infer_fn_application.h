#ifndef _LANG_TYPE_INFER_APPLICATION_H
#define _LANG_TYPE_INFER_APPLICATION_H
#include "types/type.h"
Type *infer_fn_application(Ast *ast, TypeEnv **env);
Type *infer_cons(Ast *ast, TypeEnv **env);
Type *infer_unknown_fn_signature(Ast *ast, TypeEnv **env);
#endif
