#ifndef YLC_SCRIPT_RUNTIME_H
#define YLC_SCRIPT_RUNTIME_H

#include <stdint.h>
#include <stdio.h>

#include "clap/events.h"
#include "clap/process.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ylc_program ylc_program_t;
typedef void (*ylc_midi_in_handler_fn)(int32_t channel, uint8_t event_type,
                                       int32_t note, double value);
typedef void (*ylc_param_in_handler_fn)(int32_t param_index, int32_t param_id,
                                        double value);
typedef void (*ylc_param_mod_in_handler_fn)(int32_t param_index,
                                            int32_t param_id, double amount);
typedef void (*ylc_param_gesture_in_handler_fn)(int32_t param_index,
                                                int32_t param_id,
                                                uint8_t gesture_type);

typedef struct ylc_runtime_note {
  int32_t note_id;
  int16_t port_index;
  int16_t channel;
  int16_t key;
  double velocity;
} ylc_runtime_note_t;

typedef struct ylc_program_vtable {
  void (*on_process)(void *state, const clap_audio_buffer_t *audio_inputs,
                     uint32_t audio_inputs_count,
                     clap_audio_buffer_t *audio_outputs,
                     uint32_t audio_outputs_count, uint32_t offset,
                     uint32_t frames_count, double gain);
  void (*on_note_on)(void *state, uint32_t sample_offset,
                     const ylc_runtime_note_t *note);
  void (*on_note_off)(void *state, uint32_t sample_offset,
                      const ylc_runtime_note_t *note);
  void (*on_param)(void *state, uint32_t sample_offset, clap_id param_id,
                   double value);
  void (*on_midi)(void *state, uint32_t sample_offset, uint16_t port_index,
                  const uint8_t data[3]);
  void (*on_transport)(void *state, uint32_t sample_offset,
                       const clap_event_transport_t *transport);
} ylc_program_vtable_t;

struct ylc_program {
  const ylc_program_vtable_t *vtable;
  void *state;
};

void ylc_runtime_init_bypass_program(ylc_program_t *program);
void ylc_runtime_process_span(const ylc_program_t *program,
                              const clap_process_t *process, uint32_t offset,
                              uint32_t frames_count, double gain);
void ylc_runtime_note_on(const ylc_program_t *program, uint32_t sample_offset,
                         const clap_event_note_t *event);
void ylc_runtime_note_off(const ylc_program_t *program, uint32_t sample_offset,
                          const clap_event_note_t *event);
void ylc_runtime_param(const ylc_program_t *program, uint32_t sample_offset,
                       clap_id param_id, double value);
void ylc_runtime_midi(const ylc_program_t *program, uint32_t sample_offset,
                      const clap_event_midi_t *event);
void ylc_runtime_transport(const ylc_program_t *program, uint32_t sample_offset,
                           const clap_event_transport_t *event);

void ylc_plugin_install_dummy_jit_program(void *plugin_state);
void ylc_plugin_prepare_script_audio_graph(void *plugin_state);
void ylc_plugin_set_active_audio_graph(void *plugin_state);
void ylc_plugin_clear_active_audio_graph(void *plugin_state);
void ylc_plugin_register_midi_in_handler(void *handler);
void ylc_plugin_register_param_in_handler(void *handler);
void ylc_plugin_register_param_mod_in_handler(void *handler);
void ylc_plugin_register_param_gesture_in_handler(void *handler);
void *ylc_plugin_audio_play_node(void *node);
void *ylc_plugin_audio_reset_node(void *node);
void *ylc_plugin_audio_play_voice(void *node);
void *ylc_plugin_audio_set_voice_input(int32_t input, double value, void *node);
double ylc_plugin_param_value(int32_t index);
double ylc_plugin_tempo_mul(void);
int ylc_plugin_debug_printf(const char *format, ...);
int ylc_plugin_debug_fprintf(FILE *stream, const char *format, ...);
int ylc_plugin_debug_fflush(FILE *stream);
void ylc_plugin_debug_printf_set_context(void *plugin_state);
void ylc_plugin_debug_printf_clear_context(void *plugin_state);

int ctx_sample_rate(void);

#ifdef __cplusplus
}
#endif

#endif
