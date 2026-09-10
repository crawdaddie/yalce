#ifndef YLC_CLAP_RUNTIME_SYMBOLS_H
#define YLC_CLAP_RUNTIME_SYMBOLS_H

#include "orc.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ylc_runtime_symbols_register_all(ylc_orc_session_t *orc, char *error,
                                      size_t error_size);

#ifdef __cplusplus
}
#endif

#endif
