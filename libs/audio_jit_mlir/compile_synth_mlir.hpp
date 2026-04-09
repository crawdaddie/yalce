#ifndef AUDIO_JIT_MLIR_COMPILE_SYNTH_MLIR_H
#define AUDIO_JIT_MLIR_COMPILE_SYNTH_MLIR_H

#include "synth_record.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compile an AST lambda into a JIT'd synth, add it to the LLJIT dylib,
// and register it. Returns the synth_id (>= 0) on success, -1 on failure.
// ast and ctx are Ast* / JITLangCtx* cast to void* for C header cleanliness.
int mlir_compile_synth(void *ast, const char *name, void *ctx, int num_inputs);
int mlir_compile_synth_group(void *source_ast, void *ctx, int *out_count);
int mlir_compile_synth_group_into(void *source_ast, void *ctx, int *out_count,
                                  int first_id);

// Registry access
void mlir_registry_init();
int mlir_registry_len();
MlirSynthRecord mlir_registry_get(int id);
int mlir_registry_extend(MlirSynthRecord rec);
void mlir_registry_set_ctor_ptr(int id, void *ptr);
void *mlir_registry_get_ctor_ptr(int id);
void mlir_registry_set_frame_ptr(int id, void *ptr);
void *mlir_registry_get_frame_ptr(int id);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_JIT_MLIR_COMPILE_SYNTH_MLIR_H
