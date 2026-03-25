#ifndef AUDIO_JIT_H
#define AUDIO_JIT_H

#include "../../lang/backend_llvm/common.h"

LLVMValueRef ensure_float(Type *in_type, LLVMValueRef val,
                          LLVMBuilderRef builder);

void ylc_register_synth_ctor(int synth_id, void *ctor);
void *ylc_get_synth_ctor(int synth_id);
int ylc_rand_int(int n);

extern int STYPE_AUDIO_JIT_SYM;
extern int STYPE_AUDIO_JIT_INLINE_SYM;
extern int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
extern int STYPE_AUDIO_JIT_INLINE_LAMBDA;
extern int STYPE_AUDIO_JIT_SYNTH_INLET;

#endif
