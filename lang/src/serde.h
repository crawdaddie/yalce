#ifndef _LANG_SERDE_H
#define _LANG_SERDE_H
#include "parse.h"
#include "value.h"
char *serialize_ast(Ast *ast);

void print_ast(Ast *);
char *ast_to_sexpr(Ast *, char *);

void print_value(Value *val);

#endif
