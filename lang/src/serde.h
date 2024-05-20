#ifndef _LANG_SERDE_H
#define _LANG_SERDE_H
#include "parse.h"
char *serialize_ast(Ast *ast);

void print_ast(Ast *);
char *ast_to_sexpr(Ast *, char *);
#endif
