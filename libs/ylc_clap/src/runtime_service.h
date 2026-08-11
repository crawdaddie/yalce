#ifndef YLC_RUNTIME_SERVICE_H
#define YLC_RUNTIME_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "lowering.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ylc_runtime_service ylc_runtime_service_t;

bool ylc_runtime_service_global_init(void);
void ylc_runtime_service_global_deinit(void);

ylc_runtime_service_t *ylc_runtime_service_acquire(uint32_t *instance_id);
void ylc_runtime_service_release(ylc_runtime_service_t *service,
                                 uint32_t instance_id);

uint32_t ylc_runtime_service_ref_count(const ylc_runtime_service_t *service);

bool ylc_runtime_service_compile_dummy_program(ylc_runtime_service_t *service,
                                               void *plugin_state, char *error,
                                               size_t error_size);
bool ylc_runtime_service_compile_script_program(ylc_runtime_service_t *service,
                                                void *plugin_state,
                                                const char *script_path,
                                                char *error,
                                                size_t error_size,
                                                ylc_compile_log_fn log_fn,
                                                void *log_user_data);

#ifdef __cplusplus
}
#endif

#endif
