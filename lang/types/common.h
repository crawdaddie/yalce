#ifndef _LANG_TYPES_COMMON_H
#define _LANG_TYPES_COMMON_H

#include "../parse.h"
#include <string.h>
void _print_location(Ast *ast, FILE *fstream);

#define CHARS_EQ(a, b) ((a != NULL) && (b != NULL) && strcmp(a, b) == 0)
#define IS_STRING(t)                                                           \
  t->kind == T_CONS && (t->data.T_CONS.args != NULL) &&                        \
      t->data.T_CONS.args[0]->kind ==                                          \
          T_CHAR &&CHARS_EQ(t->data.T_CONS.name, TYPE_NAME_ARRAY)

#define IS_PRIMITIVE_TYPE(t) ((1 << t->kind) & TYPE_FLAGS_PRIMITIVE)
// || IS_STRING(t)
#endif
