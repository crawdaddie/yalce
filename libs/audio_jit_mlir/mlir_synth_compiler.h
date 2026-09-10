#ifndef YLC_AUDIO_JIT_MLIR_SYNTH_COMPILER_H
#define YLC_AUDIO_JIT_MLIR_SYNTH_COMPILER_H

#include "audio_jit_mlir.h"

#include <string>

namespace ylc::audio_mlir {

struct MlirSynthNames {
  std::string public_name;
  std::string prefix;
  std::string cons;
  std::string init;
  std::string perform;
  std::string frame;
};

struct MlirSynthCompileResult {
  bool ok = false;
  MlirSynthNames names;
  LLVMValueRef cons_fn = nullptr;
  LLVMValueRef init_fn = nullptr;
  LLVMValueRef perform_fn = nullptr;
  LLVMValueRef frame_fn = nullptr;
  unsigned arg_count = 0;
  int output_lanes = 1;
  int state_bytes = 0;
};

MlirSynthCompileResult compile_mlir_synth_stub(Ast *lambda,
                                               const MlirSynthNames &names,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder);

} // namespace ylc::audio_mlir

#endif
