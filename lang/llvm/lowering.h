#ifndef _LANG_MIR_TO_LLVM_LOWERING_H
#define _LANG_MIR_TO_LLVM_LOWERING_H
#include "../mir/mir.h"
#include <llvm-c/Types.h>

LLVMValueRef lower_mir(MirProgram *prog, LLVMModuleRef module,
                       LLVMBuilderRef builder);
#endif
