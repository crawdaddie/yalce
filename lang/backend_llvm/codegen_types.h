#ifndef _LANG_BACKEND_LLVM_CODEGEN_TYPES_H
#define _LANG_BACKEND_LLVM_CODEGEN_TYPES_H
#include "types/type.h"
#include "llvm-c/Types.h"
LLVMTypeRef type_to_llvm_type(Type *type);

#endif
