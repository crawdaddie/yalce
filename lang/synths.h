#ifndef _LANG_SYNTHS_H
#define _LANG_SYNTHS_H
// this module contains non-first-class utilities for working with Synth nodes
// from the synth engine
#include "backend_llvm/common.h"
#include "types/type.h"
#include "llvm-c/Types.h"
#define TYPE_NAME_SYNTH "synth"
extern Type t_synth;

void initialize_synth_types(JITLangCtx *ctx, LLVMModuleRef module, LLVMBuilderRef builder);

#endif
