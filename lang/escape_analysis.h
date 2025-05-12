#ifndef _LANG_ESCAPE_ANALYSIS_H
#define _LANG_ESCAPE_ANALYSIS_H
#include "parse.h"

// typedef struct RefGraph {
//   Ast *start;
//   Ast *this;
//   Ast *next;
//   Ast *prev;
// } RefGraph;

typedef struct EscapesEnv {
  const char *varname;
  Ast *expr;
  uint32_t id;
  struct EscapesEnv *next;
} EscapesEnv;

typedef struct AECtx {
  int scope;
  EscapesEnv *env;

} AECtx;

void escape_analysis(Ast *prog, AECtx *ctx);

#endif
