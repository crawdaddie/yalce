#ifndef YLC_SCRIPT_JIT_H
#define YLC_SCRIPT_JIT_H

#include <stdbool.h>
#include <stddef.h>

#include "lowering.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ylc_script_jit ylc_script_jit_t;

ylc_script_jit_t *ylc_script_jit_create(char *error, size_t error_size);
void ylc_script_jit_destroy(ylc_script_jit_t *jit);

bool ylc_script_jit_compile_dummy_program(ylc_script_jit_t *jit,
                                          void *plugin_state, char *error,
                                          size_t error_size);
bool ylc_script_jit_compile_script_program(ylc_script_jit_t *jit,
                                           void *plugin_state,
                                           const char *script_path,
                                           char *error, size_t error_size,
                                           ylc_compile_log_fn log_fn,
                                           void *log_user_data);

#ifdef __cplusplus
}
#endif

#endif
