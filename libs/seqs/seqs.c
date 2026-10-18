#include "../../lang/ylc_datatypes.h"

__attribute__((constructor)) static void ylc_seq_init(void) {
  // if (!ylc_mir_program_init_fn) {
  //   ylc_mir_program_init_fn = ylc_audio_jit_register_program;
  // }
  //
  // ylc_audio_jit_register_current_program(NULL);
  //
  // TypeEnv audio_tenv = {.name = "Audio"};
  // mir_register_builtin(ylc_mir_program, &audio_tenv, MirCompileAudioHandler,
  //                      MIR_BUILTIN_SYMBOL_EXTENSION, NULL, 0,
  //                      MIR_RESULT_NONE);
}
