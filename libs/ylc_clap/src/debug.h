#ifndef YLC_CLAP_PLUGIN_DEBUG_H
#define YLC_CLAP_PLUGIN_DEBUG_H

#include "plugin_internal.h"
void ylc_debug_log(ylc_plugin_t *self, const char *format, ...);

void ylc_debug_compile_log(void *user_data, const char *line);

extern _Thread_local ylc_plugin_t *ylc_debug_printf_context;

void ylc_debug_drain_pipe(ylc_plugin_t *self);

void ylc_register_debug_pipe(ylc_plugin_t *self);

void ylc_close_debug_pipe(ylc_plugin_t *self);

void ylc_close_debug_log_file(ylc_plugin_t *self);

void ylc_open_debug_pipe(ylc_plugin_t *self);

bool ylc_open_debug_log_file(ylc_plugin_t *self, const char *path,
                             int *open_errno);
#endif
