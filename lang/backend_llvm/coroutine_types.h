#ifndef _LANG_BACKEND_LLVM_COROUTINE_TYPES_H
#define _LANG_BACKEND_LLVM_COROUTINE_TYPES_H
#include "llvm-c/Types.h"

LLVMTypeRef coroutine_def_fn_type(LLVMTypeRef instance_type,
                                  LLVMTypeRef ret_option_type);

LLVMTypeRef coroutine_instance_type(LLVMTypeRef params_obj_type);
#endif
