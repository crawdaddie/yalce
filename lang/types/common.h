#ifndef _LANG_TYPES_COMMON_H
#define _LANG_TYPES_COMMON_H

#include "../parse.h"
#include <string.h>
void _print_location(Ast *ast, FILE *fstream);

#define IS_PRIMITIVE_TYPE(t) ((1 << t->kind) & TYPE_FLAGS_PRIMITIVE)
#define CHARS_EQ(a, b) ((a != NULL) && (b != NULL) && strcmp(a, b) == 0)
#endif
