#ifndef _LANG_ESCAPE_ANALYSIS_H
#define _LANG_ESCAPE_ANALYSIS_H

#include "parse.h"
typedef struct AECtx {
} AECtx;
void escape_analysis(Ast *prog, AECtx *ctx);

#endif
