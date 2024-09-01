#ifndef _LANG_TYPE_INFERENCE_APPLICATION_H
#define _LANG_TYPE_INFERENCE_APPLICATION_H
#include "parse.h"
#include "types/type.h"
Type *infer_fn_application(TypeEnv **env, Ast *ast);

typedef struct TypeMap {
  Type *key;
  Type *type;
  struct TypeMap *next;
} TypeMap;

#endif
