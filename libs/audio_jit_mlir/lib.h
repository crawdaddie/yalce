#ifndef _AUDIO_JIT_MLIR_LIB_H
#define _AUDIO_JIT_MLIR_LIB_H
extern int STYPE_AUDIO_JIT_SYM;
extern int STYPE_AUDIO_JIT_INLINE_SYM;
extern int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
extern int STYPE_AUDIO_JIT_INLINE_LAMBDA;
extern int STYPE_AUDIO_JIT_SYNTH_INLET;
extern int STYPE_AUDIO_JIT_LOCAL_ARRAY;

void ylc_mlir_set_global_storage(void **storage_array, int *storage_capacity);
double ylc_mlir_get_global_f64(int slot);
int ylc_mlir_get_global_i32(int slot);
int ylc_mlir_get_global_i1(int slot);
#endif
