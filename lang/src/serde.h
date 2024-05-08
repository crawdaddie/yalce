#ifndef _LANG_SERDE_H
#define _LANG_SERDE_H
#include "parse.h"
char *serialize_ast(Ast *ast);

void print_ser_ast(Ast *ast);
#endif
