#include "runtime_symbols.h"

#include "audio_graph.h"
#include "scheduler.h"
#include "script_runtime.h"
#include "soundfile.h"
#include "ylc_stdlib.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ylc_runtime_symbol {
  const char *name;
  void *address;
} ylc_runtime_symbol_t;

extern void *global_storage_array[];
extern int global_storage_size;

extern double nonzero_randu_double(void);
extern DoublePair _randn_pair(double mu, double sigma);
extern int ilog2(long long x);
extern long long u64pow(long long x, long long ex);
extern int ipow(int base, int exp);
extern void *ylc_get_output_buf(void *node_raw);
extern void *ylc_audio_node_inline_state(void *node_raw);
extern void ylc_audio_memzero(void *ptr, int32_t size);
extern Node *ylc_create_audio_frame_node(frame_perform_func_t frame_perform,
                                         int num_inputs, int output_layout,
                                         int state_bytes,
                                         const char *meta_name);
extern void ylc_audio_node_set_state_init(void *node_raw, void *init_raw);
extern Node *ylc_audio_graph_create_scalar_node(double value);
extern Node *ylc_audio_graph_set_input_scalar(Node *node, int input,
                                              double value);
extern double ylc_read_inlet_node_i32(void *node_raw, int frame);

static int ylc_runtime_symbol_round_int(double x) { return (int)round(x); }

static int ylc_runtime_symbol_ceil_int(double x) { return (int)ceil(x); }

static int ylc_runtime_symbol_ilog2_int(int x) {
  return ilog2((long long)x);
}

static double ylc_runtime_symbol_midi_to_freq(int32_t note) {
  return 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
}

static const ylc_runtime_symbol_t ylc_plugin_symbols[] = {
    {"ylc_plugin_install_dummy_jit_program",
     (void *)&ylc_plugin_install_dummy_jit_program},
    {"printf", (void *)&ylc_plugin_debug_printf},
    {"fprintf", (void *)&ylc_plugin_debug_fprintf},
    {"fflush", (void *)&ylc_plugin_debug_fflush},
    {"MidiIn", (void *)&ylc_plugin_register_midi_in_handler},
    {"ParamIn", (void *)&ylc_plugin_register_param_in_handler},
    {"ParamModIn", (void *)&ylc_plugin_register_param_mod_in_handler},
    {"ParamGestureIn", (void *)&ylc_plugin_register_param_gesture_in_handler},
    {"play_node", (void *)&ylc_plugin_audio_play_node},
    {"reset_node", (void *)&ylc_plugin_audio_reset_node},
    {"play_voice", (void *)&ylc_plugin_audio_play_voice},
    {"set_voice_input", (void *)&ylc_plugin_audio_set_voice_input},
    {"param", (void *)&ylc_plugin_param_value},
    {"ylc_plugin_param_value", (void *)&ylc_plugin_param_value},
    {"ylc_plugin_persist_array", (void *)&ylc_plugin_persist_array},
    {"ylc_plugin_soundfile_ui", (void *)&ylc_plugin_soundfile_ui},
    {"sf_channels", (void *)&ylc_plugin_sf_channels},
    {"sf_samplerate", (void *)&ylc_plugin_sf_samplerate},
    {"sf_data", (void *)&ylc_plugin_sf_data},
    {"EnvArrayUI", (void *)&EnvArrayUI},
    {"ADSRArrayUI", (void *)&ADSRArrayUI},
    {"tempo_mul", (void *)&ylc_plugin_tempo_mul},
    {"tempo_coeff", (void *)&ylc_plugin_tempo_mul},
    {"ylc_plugin_tempo_mul", (void *)&ylc_plugin_tempo_mul},
    {"midi_to_freq", (void *)&ylc_runtime_symbol_midi_to_freq},
    {"schedule_event", (void *)&ylc_clap_schedule_event},
    {"ylc_clap_schedule_current_task_event",
     (void *)&ylc_clap_schedule_current_task_event},
    {"ylc_clap_complete_current_task",
     (void *)&ylc_clap_complete_current_task},
    {"ylc_play_pattern_start", (void *)&ylc_clap_play_pattern_start},
    {"get_current_sample", (void *)&ylc_clap_get_current_sample},
    {"get_tl_tick", (void *)&ylc_clap_get_tl_tick},
    {"get_sched_tick", (void *)&ylc_clap_get_sched_tick},
    {"ctx_sample_rate", (void *)&ctx_sample_rate},
    {"cancel_task", (void *)&ylc_clap_cancel_task},
};

static const ylc_runtime_symbol_t ylc_audio_jit_symbols[] = {
    {"const_sig", (void *)&ylc_audio_graph_create_scalar_node},
    {"ylc_create_audio_frame_node", (void *)&ylc_create_audio_frame_node},
    {"ylc_audio_node_set_state_init",
     (void *)&ylc_audio_node_set_state_init},
    {"ylc_audio_graph_create_scalar_node",
     (void *)&ylc_audio_graph_create_scalar_node},
    {"ylc_audio_graph_set_input_scalar",
     (void *)&ylc_audio_graph_set_input_scalar},
    {"ylc_audio_node_inline_state", (void *)&ylc_audio_node_inline_state},
    {"ylc_get_output_buf", (void *)&ylc_get_output_buf},
    {"ylc_read_inlet_node", (void *)&ylc_read_inlet_node},
    {"ylc_read_inlet_node_i32", (void *)&ylc_read_inlet_node_i32},
    {"ylc_audio_memzero", (void *)&ylc_audio_memzero},
    {"node_connect_input", (void *)&node_connect_input},
};

static const ylc_runtime_symbol_t ylc_stdlib_symbols[] = {
    {"__ylc_dup", (void *)&__ylc_dup},
    {"__ylc_drop", (void *)&__ylc_drop},
    {"rand_int", (void *)&rand_int},
    {"rand_double", (void *)&rand_double},
    {"rand_double_range", (void *)&rand_double_range},
    {"amp_db", (void *)&amp_db},
    {"db_amp", (void *)&db_amp},
    {"bipolar_scale", (void *)&bipolar_scale},
    {"unipolar_scale", (void *)&unipolar_scale},
    {"nonzero_randu_double", (void *)&nonzero_randu_double},
    {"_randn_pair", (void *)&_randn_pair},
    {"matrix_vec_mul_double", (void *)&matrix_vec_mul_double},
    {"vec_dot_double", (void *)&vec_dot_double},
    {"u64pow", (void *)&u64pow},
    {"ipow", (void *)&ipow},
};

static const ylc_runtime_symbol_t ylc_math_symbols[] = {
    {"sin", (void *)&sin},
    {"cos", (void *)&cos},
    {"exp", (void *)&exp},
    {"fmod", (void *)&fmod},
    {"pow", (void *)&pow},
    {"tanh", (void *)&tanh},
    {"tan", (void *)&tan},
    {"atan", (void *)&atan},
    {"atan2", (void *)&atan2},
    {"log", (void *)&log},
    {"sqrt", (void *)&sqrt},
    {"floor", (void *)&floor},
    {"round", (void *)&ylc_runtime_symbol_round_int},
    {"ceil", (void *)&ylc_runtime_symbol_ceil_int},
    {"ilog2", (void *)&ylc_runtime_symbol_ilog2_int},
};

static const ylc_runtime_symbol_t ylc_libc_symbols[] = {
    {"rand", (void *)&rand},
    {"snprintf", (void *)&snprintf},
    {"strlen", (void *)&strlen},
};

static bool ylc_runtime_symbols_register_table(
    ylc_orc_session_t *orc, const ylc_runtime_symbol_t *symbols, size_t count,
    char *error, size_t error_size) {
  if (!orc || !symbols) {
    return false;
  }

  for (size_t i = 0; i < count; ++i) {
    if (!ylc_orc_session_define_host_symbol(orc, symbols[i].name,
                                            symbols[i].address, error,
                                            error_size)) {
      return false;
    }
  }

  return true;
}

bool ylc_runtime_symbols_register_all(ylc_orc_session_t *orc, char *error,
                                      size_t error_size) {
#define YLC_ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

  if (!ylc_runtime_symbols_register_table(
          orc, ylc_plugin_symbols, YLC_ARRAY_LEN(ylc_plugin_symbols), error,
          error_size)) {
    return false;
  }

  if (!ylc_runtime_symbols_register_table(
          orc, ylc_audio_jit_symbols, YLC_ARRAY_LEN(ylc_audio_jit_symbols),
          error, error_size)) {
    return false;
  }

  if (!ylc_runtime_symbols_register_table(
          orc, ylc_stdlib_symbols, YLC_ARRAY_LEN(ylc_stdlib_symbols), error,
          error_size)) {
    return false;
  }

  if (!ylc_runtime_symbols_register_table(
          orc, ylc_math_symbols, YLC_ARRAY_LEN(ylc_math_symbols), error,
          error_size)) {
    return false;
  }

  if (!ylc_runtime_symbols_register_table(
          orc, ylc_libc_symbols, YLC_ARRAY_LEN(ylc_libc_symbols), error,
          error_size)) {
    return false;
  }

  if (!ylc_orc_session_define_host_data_symbol(
          orc, "global_storage_array", global_storage_array, error,
          error_size)) {
    return false;
  }

  return ylc_orc_session_define_host_data_symbol(
      orc, "global_storage_size", &global_storage_size, error, error_size);

#undef YLC_ARRAY_LEN
}
