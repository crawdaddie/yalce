#ifndef YLC_CLAP_LOWERING_H
#define YLC_CLAP_LOWERING_H

#include "orc.h"

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ylc_script_entry_return_kind {
  YLC_SCRIPT_ENTRY_RET_UNSUPPORTED,
  YLC_SCRIPT_ENTRY_RET_VOID,
  YLC_SCRIPT_ENTRY_RET_I32,
  YLC_SCRIPT_ENTRY_RET_DOUBLE,
  YLC_SCRIPT_ENTRY_RET_PTR,
} ylc_script_entry_return_kind_t;

typedef struct ylc_lowered_script {
  LLVMContextRef context;
  LLVMModuleRef module;
  char entry_name[96];
  ylc_script_entry_return_kind_t entry_return_kind;
  bool entry_returns_void;
} ylc_lowered_script_t;

typedef void (*ylc_compile_log_fn)(void *user_data, const char *line);

bool ylc_lower_dummy_installer_module(ylc_orc_session_t *orc,
                                      uint64_t module_id,
                                      char *installer_name,
                                      size_t installer_name_size,
                                      LLVMContextRef *context_out,
                                      LLVMModuleRef *module_out, char *error,
                                      size_t error_size);
bool ylc_lower_script_file(ylc_orc_session_t *orc, uint64_t module_id,
                           const char *script_path,
                           ylc_lowered_script_t *compiled, char *error,
                           size_t error_size, ylc_compile_log_fn log_fn,
                           void *log_user_data);

#ifdef __cplusplus
}
#endif

#endif
