#ifndef _LANG_PARSER_H
#define _LANG_PARSER_H
#include "ast_node.h"
NBlock *parse(const char *input);
void print_ast(NBlock *block);
#endif
