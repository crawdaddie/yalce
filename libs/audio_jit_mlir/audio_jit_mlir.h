#ifndef YLC_AUDIO_JIT_MLIR_H
#define YLC_AUDIO_JIT_MLIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../lang/backend_llvm/common.h"

extern int STYPE_AUDIO_JIT_MLIR_SYM;

LLVMValueRef CompileAudioMLIRFnHandler(Ast *ast, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder);

void ylc_audio_mlir_register_synth_ctor(int synth_id, void *ctor);
void *ylc_audio_mlir_get_synth_ctor(int synth_id);

void *ylc_audio_mlir_create_audio_node(void *perform, int num_inputs,
                                       int output_layout, int state_bytes,
                                       const char *meta_name);
void *ylc_audio_mlir_node_state(void *node);
void *ylc_audio_mlir_get_output_buf(void *node);
double ylc_audio_mlir_read_inlet_node(void *node, int64_t frame);
void *ylc_audio_mlir_const_inlet(double val);

#ifdef __cplusplus
}
#endif

#endif
