#ifndef _LANG_BACKEND_NATIVE_FUNCTIONS_H
#define _LANG_BACKEND_NATIVE_FUNCTIONS_H

#include "../type_inference.h"
#include "ht.h"
#include "llvm-c/Types.h"
void _add_native_functions(ht *stack, LLVMModuleRef module, TypeEnv *env);

#endif
