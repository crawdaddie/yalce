#ifndef _LANG_COMPILER_H
#define _LANG_COMPILER_H
#include "obj.h"
#include "vm.h"
bool compile(const char *source, Chunk *chunk);
#endif
