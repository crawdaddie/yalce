#include "script_jit.h"

#include "lowering.h"
#include "orc.h"
#include "runtime_symbols.h"
#include "script_runtime.h"

#include <llvm-c/Orc.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*ylc_jit_install_fn)(void *plugin_state);
typedef void (*ylc_script_top_void_fn)(void);
typedef int32_t (*ylc_script_top_i32_fn)(void);
typedef double (*ylc_script_top_double_fn)(void);
typedef void *(*ylc_script_top_ptr_fn)(void);

struct ylc_script_jit {
  ylc_orc_session_t *orc;
  uint64_t next_module_id;
};

ylc_script_jit_t *ylc_script_jit_create(char *error, size_t error_size) {
  ylc_script_jit_t *jit = (ylc_script_jit_t *)calloc(1, sizeof(*jit));
  if (!jit) {
    return NULL;
  }

  jit->orc = ylc_orc_session_create(error, error_size);
  if (!jit->orc) {
    free(jit);
    return NULL;
  }

  jit->next_module_id = 1;

  if (!ylc_runtime_symbols_register_all(jit->orc, error, error_size)) {
    ylc_script_jit_destroy(jit);
    return NULL;
  }

  return jit;
}

void ylc_script_jit_destroy(ylc_script_jit_t *jit) {
  if (!jit) {
    return;
  }

  ylc_orc_session_destroy(jit->orc);
  free(jit);
}

bool ylc_script_jit_compile_dummy_program(ylc_script_jit_t *jit,
                                          void *plugin_state, char *error,
                                          size_t error_size) {
  if (!jit || !jit->orc || !plugin_state) {
    return false;
  }

  char installer_name[96] = {0};
  LLVMContextRef context = NULL;
  LLVMModuleRef module = NULL;
  const uint64_t module_id = jit->next_module_id++;
  if (!ylc_lower_dummy_installer_module(jit->orc, module_id, installer_name,
                                        sizeof(installer_name), &context,
                                        &module, error, error_size)) {
    return false;
  }

  if (!ylc_orc_session_add_module(jit->orc, context, module, error,
                                  error_size)) {
    return false;
  }

  LLVMOrcExecutorAddress address = 0;
  if (!ylc_orc_session_lookup(jit->orc, installer_name, &address, error,
                              error_size)) {
    return false;
  }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
  ylc_jit_install_fn install = (ylc_jit_install_fn)(uintptr_t)address;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  install(plugin_state);
  return true;
}

bool ylc_script_jit_compile_script_program(ylc_script_jit_t *jit,
                                           void *plugin_state,
                                           const char *script_path,
                                           char *error, size_t error_size,
                                           ylc_compile_log_fn log_fn,
                                           void *log_user_data) {
  if (!jit || !jit->orc || !plugin_state || !script_path ||
      script_path[0] == '\0') {
    if (error && error_size > 0) {
      snprintf(error, error_size, "invalid script compile request");
    }
    return false;
  }

  ylc_lowered_script_t compiled = {0};
  const uint64_t module_id = jit->next_module_id++;
  ylc_plugin_debug_printf_set_context(plugin_state);
  if (!ylc_lower_script_file(jit->orc, module_id, script_path, &compiled,
                             error, error_size, log_fn, log_user_data)) {
    ylc_plugin_debug_printf_clear_context(plugin_state);
    return false;
  }
  ylc_plugin_debug_printf_clear_context(plugin_state);

  if (!ylc_orc_session_add_module(jit->orc, compiled.context, compiled.module,
                                  error, error_size)) {
    return false;
  }

  LLVMOrcExecutorAddress address = 0;
  if (!ylc_orc_session_lookup(jit->orc, compiled.entry_name, &address, error,
                              error_size)) {
    return false;
  }

  if (compiled.entry_return_kind != YLC_SCRIPT_ENTRY_RET_UNSUPPORTED) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    ylc_plugin_prepare_script_audio_graph(plugin_state);
    ylc_plugin_set_active_audio_graph(plugin_state);
    switch (compiled.entry_return_kind) {
    case YLC_SCRIPT_ENTRY_RET_VOID: {
      ylc_script_top_void_fn top = (ylc_script_top_void_fn)(uintptr_t)address;
      ylc_plugin_debug_printf_set_context(plugin_state);
      top();
      ylc_plugin_debug_printf_clear_context(plugin_state);
      break;
    }
    case YLC_SCRIPT_ENTRY_RET_I32: {
      ylc_script_top_i32_fn top = (ylc_script_top_i32_fn)(uintptr_t)address;
      ylc_plugin_debug_printf_set_context(plugin_state);
      (void)top();
      ylc_plugin_debug_printf_clear_context(plugin_state);
      break;
    }
    case YLC_SCRIPT_ENTRY_RET_DOUBLE: {
      ylc_script_top_double_fn top =
          (ylc_script_top_double_fn)(uintptr_t)address;
      ylc_plugin_debug_printf_set_context(plugin_state);
      (void)top();
      ylc_plugin_debug_printf_clear_context(plugin_state);
      break;
    }
    case YLC_SCRIPT_ENTRY_RET_PTR: {
      ylc_script_top_ptr_fn top = (ylc_script_top_ptr_fn)(uintptr_t)address;
      ylc_plugin_debug_printf_set_context(plugin_state);
      (void)top();
      ylc_plugin_debug_printf_clear_context(plugin_state);
      break;
    }
    case YLC_SCRIPT_ENTRY_RET_UNSUPPORTED:
    default:
      break;
    }
    ylc_plugin_clear_active_audio_graph(plugin_state);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  }

  return ylc_script_jit_compile_dummy_program(jit, plugin_state, error,
                                              error_size);
}
