#ifndef SRET_TRANSFORM_H
#define SRET_TRANSFORM_H

#include "llvm-c/Core.h"

// Transform all calls to extern functions with large struct returns to use sret
void transform_sret_calls(LLVMModuleRef module);

// Transform a single call instruction to use sret
void transform_call_to_sret(LLVMValueRef call_inst, LLVMValueRef callee,
                             LLVMTypeRef ret_type, LLVMTypeRef fn_type,
                             LLVMModuleRef module);

#endif // SRET_TRANSFORM_H
