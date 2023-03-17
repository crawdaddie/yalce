#ifndef _LANG_COMPILER_H
#define _LANG_COMPILER_H
#include "obj_function.h"
#include "vm.h"

ObjFunction *compile(char *source, Chunk *chunk);
#endif
