#include "clap/entry.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/gui.h"
#include "clap/ext/note-ports.h"
#include "clap/ext/params.h"
#include "clap/ext/posix-fd-support.h"
#include "clap/ext/state.h"
#include "clap/factory/plugin-factory.h"

#include "debug.h"
#include "plugin_internal.h"
#include "runtime_service.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct ylc_saved_state_header {
  uint32_t magic;
  uint32_t version;
  double param_values[YLC_PARAM_COUNT];
  uint32_t script_path_len;
} ylc_saved_state_header_t;

typedef struct ylc_persistent_array_rc_header {
  uint32_t rc;
  uint32_t tag_or_size_class;
} ylc_persistent_array_rc_header_t;

enum {
  YLC_MIDI_EVENT_TYPE_NOTE_ON = 0,
  YLC_MIDI_EVENT_TYPE_NOTE_OFF = 1,
  YLC_MIDI_EVENT_TYPE_CC = 2,
};

enum {
  YLC_PARAM_GESTURE_TYPE_BEGIN = 0,
  YLC_PARAM_GESTURE_TYPE_END = 1,
};

ylc_plugin_t *ylc_from_plugin(const clap_plugin_t *plugin) {
  return plugin ? (ylc_plugin_t *)plugin->plugin_data : NULL;
}

static void
ylc_queue_input_event_log(ylc_plugin_t *self,
                          const ylc_input_event_log_record_t *record) {
  if (!self || !record) {
    return;
  }

  const unsigned int write_seq = atomic_load_explicit(
      &self->input_event_log_write_seq, memory_order_relaxed);
  const unsigned int read_seq = atomic_load_explicit(
      &self->input_event_log_read_seq, memory_order_acquire);

  if (write_seq - read_seq >= YLC_INPUT_EVENT_LOG_CAPACITY) {
    atomic_fetch_add_explicit(&self->input_event_log_dropped, 1,
                              memory_order_relaxed);
    return;
  }

  self->input_event_log[write_seq % YLC_INPUT_EVENT_LOG_CAPACITY] = *record;
  atomic_store_explicit(&self->input_event_log_write_seq, write_seq + 1,
                        memory_order_release);
}

static void ylc_handle_note_event(void *user_data,
                                  const clap_event_note_t *event,
                                  ylc_input_event_log_type_t type) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = type,
      .sample_offset = event->header.time,
      .clap_type = event->header.type,
      .note_id = event->note_id,
      .port_index = event->port_index,
      .channel = event->channel,
      .key = event->key,
      .value = event->velocity,
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_handle_note_on(void *user_data,
                               const clap_event_note_t *event) {
  (void)user_data;
  (void)event;
}

static void ylc_handle_note_off(void *user_data,
                                const clap_event_note_t *event) {
  (void)user_data;
  (void)event;
}

static void ylc_handle_note_choke(void *user_data,
                                  const clap_event_note_t *event) {
  ylc_handle_note_event(user_data, event, YLC_INPUT_EVENT_NOTE_CHOKE);
}

static void
ylc_handle_note_expression(void *user_data,
                           const clap_event_note_expression_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = YLC_INPUT_EVENT_NOTE_EXPRESSION,
      .sample_offset = event->header.time,
      .clap_type = event->header.type,
      .note_id = event->note_id,
      .port_index = event->port_index,
      .channel = event->channel,
      .key = event->key,
      .value = event->value,
      .size = (uint32_t)event->expression_id,
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_handle_param_value(void *user_data,
                                   const clap_event_param_value_t *event) {
  (void)user_data;
  (void)event;
}

static void ylc_handle_param_mod(void *user_data,
                                 const clap_event_param_mod_t *event) {
  (void)user_data;
  (void)event;
}

static void ylc_handle_param_gesture(void *user_data,
                                     const clap_event_param_gesture_t *event) {
  (void)user_data;
  (void)event;
}

static void ylc_handle_transport(void *user_data,
                                 const clap_event_transport_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = YLC_INPUT_EVENT_TRANSPORT,
      .sample_offset = event->header.time,
      .clap_type = event->header.type,
      .value = event->tempo,
      .size = event->flags,
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_handle_midi(void *user_data, const clap_event_midi_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }
}

static void ylc_handle_midi_sysex(void *user_data,
                                  const clap_event_midi_sysex_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = YLC_INPUT_EVENT_MIDI_SYSEX,
      .sample_offset = event->header.time,
      .clap_type = event->header.type,
      .port_index = (int16_t)event->port_index,
      .size = event->size,
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_handle_midi2(void *user_data, const clap_event_midi2_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = YLC_INPUT_EVENT_MIDI2,
      .sample_offset = event->header.time,
      .clap_type = event->header.type,
      .port_index = (int16_t)event->port_index,
      .size = event->data[0],
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_handle_unknown(void *user_data,
                               const clap_event_header_t *event) {
  ylc_plugin_t *self = (ylc_plugin_t *)user_data;
  if (!self || !event) {
    return;
  }

  const ylc_input_event_log_record_t record = {
      .type = YLC_INPUT_EVENT_UNKNOWN,
      .sample_offset = event->time,
      .clap_type = event->type,
      .size = event->size,
  };
  ylc_queue_input_event_log(self, &record);
}

static void ylc_event_handlers_init(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  self->event_handlers = (ylc_event_handlers_t){
      .user_data = self,
      .on_note_on = ylc_handle_note_on,
      .on_note_off = ylc_handle_note_off,
      .on_note_choke = ylc_handle_note_choke,
      .on_note_expression = ylc_handle_note_expression,
      .on_param_value = ylc_handle_param_value,
      .on_param_mod = ylc_handle_param_mod,
      .on_param_gesture = ylc_handle_param_gesture,
      .on_transport = ylc_handle_transport,
      .on_midi = ylc_handle_midi,
      .on_midi_sysex = ylc_handle_midi_sysex,
      .on_midi2 = ylc_handle_midi2,
      .on_unknown = ylc_handle_unknown,
  };
}

static void ylc_event_handlers_dispatch(ylc_event_handlers_t *handlers,
                                        const clap_event_header_t *header) {
  if (!handlers || !header || header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
    return;
  }

  switch (header->type) {
  case CLAP_EVENT_NOTE_ON:
    if (handlers->on_note_on) {
      handlers->on_note_on(handlers->user_data,
                           (const clap_event_note_t *)header);
    }
    break;
  case CLAP_EVENT_NOTE_OFF:
    if (handlers->on_note_off) {
      handlers->on_note_off(handlers->user_data,
                            (const clap_event_note_t *)header);
    }
    break;
  case CLAP_EVENT_NOTE_CHOKE:
    if (handlers->on_note_choke) {
      handlers->on_note_choke(handlers->user_data,
                              (const clap_event_note_t *)header);
    }
    break;
  case CLAP_EVENT_NOTE_EXPRESSION:
    if (handlers->on_note_expression) {
      handlers->on_note_expression(
          handlers->user_data, (const clap_event_note_expression_t *)header);
    }
    break;
  case CLAP_EVENT_PARAM_VALUE:
    if (handlers->on_param_value) {
      handlers->on_param_value(handlers->user_data,
                               (const clap_event_param_value_t *)header);
    }
    break;
  case CLAP_EVENT_PARAM_MOD:
    if (handlers->on_param_mod) {
      handlers->on_param_mod(handlers->user_data,
                             (const clap_event_param_mod_t *)header);
    }
    break;
  case CLAP_EVENT_PARAM_GESTURE_BEGIN:
  case CLAP_EVENT_PARAM_GESTURE_END:
    if (handlers->on_param_gesture) {
      handlers->on_param_gesture(handlers->user_data,
                                 (const clap_event_param_gesture_t *)header);
    }
    break;
  case CLAP_EVENT_TRANSPORT:
    if (handlers->on_transport) {
      handlers->on_transport(handlers->user_data,
                             (const clap_event_transport_t *)header);
    }
    break;
  case CLAP_EVENT_MIDI:
    if (handlers->on_midi) {
      handlers->on_midi(handlers->user_data, (const clap_event_midi_t *)header);
    }
    break;
  case CLAP_EVENT_MIDI_SYSEX:
    if (handlers->on_midi_sysex) {
      handlers->on_midi_sysex(handlers->user_data,
                              (const clap_event_midi_sysex_t *)header);
    }
    break;
  case CLAP_EVENT_MIDI2:
    if (handlers->on_midi2) {
      handlers->on_midi2(handlers->user_data,
                         (const clap_event_midi2_t *)header);
    }
    break;
  default:
    if (handlers->on_unknown) {
      handlers->on_unknown(handlers->user_data, header);
    }
    break;
  }
}

static void ylc_drain_input_event_log(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  const unsigned int dropped = atomic_exchange_explicit(
      &self->input_event_log_dropped, 0, memory_order_acq_rel);
  if (dropped > 0) {
    char line[YLC_DEBUG_LINE_SIZE] = {0};
    snprintf(line, sizeof(line), "input events dropped: %u", dropped);
    ylc_debug_log(self, "%s", line);
  }

  unsigned int read_seq = atomic_load_explicit(&self->input_event_log_read_seq,
                                               memory_order_relaxed);
  const unsigned int write_seq = atomic_load_explicit(
      &self->input_event_log_write_seq, memory_order_acquire);

  while (read_seq != write_seq) {
    const ylc_input_event_log_record_t record =
        self->input_event_log[read_seq % YLC_INPUT_EVENT_LOG_CAPACITY];
    char line[YLC_DEBUG_LINE_SIZE] = {0};

    switch (record.type) {
    case YLC_INPUT_EVENT_NOTE_ON:
      snprintf(line, sizeof(line), "in note_on t=%u ch=%d key=%d vel=%.3f",
               record.sample_offset, record.channel, record.key, record.value);
      break;
    case YLC_INPUT_EVENT_NOTE_OFF:
      snprintf(line, sizeof(line), "in note_off t=%u ch=%d key=%d vel=%.3f",
               record.sample_offset, record.channel, record.key, record.value);
      break;
    case YLC_INPUT_EVENT_NOTE_CHOKE:
      snprintf(line, sizeof(line), "in note_choke t=%u ch=%d key=%d",
               record.sample_offset, record.channel, record.key);
      break;
    case YLC_INPUT_EVENT_NOTE_EXPRESSION:
      snprintf(line, sizeof(line), "in note_expr t=%u expr=%u value=%.3f",
               record.sample_offset, record.size, record.value);
      break;
    case YLC_INPUT_EVENT_PARAM_VALUE:
      snprintf(line, sizeof(line), "in param_value t=%u id=%u value=%.3f",
               record.sample_offset, record.param_id, record.value);
      break;
    case YLC_INPUT_EVENT_PARAM_MOD:
      snprintf(line, sizeof(line), "in param_mod t=%u id=%u amount=%.3f",
               record.sample_offset, record.param_id, record.value);
      break;
    case YLC_INPUT_EVENT_PARAM_GESTURE_BEGIN:
      snprintf(line, sizeof(line), "in param_gesture_begin t=%u id=%u",
               record.sample_offset, record.param_id);
      break;
    case YLC_INPUT_EVENT_PARAM_GESTURE_END:
      snprintf(line, sizeof(line), "in param_gesture_end t=%u id=%u",
               record.sample_offset, record.param_id);
      break;
    case YLC_INPUT_EVENT_TRANSPORT:
      snprintf(line, sizeof(line), "in transport t=%u flags=0x%x tempo=%.3f",
               record.sample_offset, record.size, record.value);
      break;
    case YLC_INPUT_EVENT_MIDI:
      snprintf(line, sizeof(line), "in midi t=%u port=%d %02x %02x %02x",
               record.sample_offset, record.port_index, record.midi[0],
               record.midi[1], record.midi[2]);
      break;
    case YLC_INPUT_EVENT_MIDI_SYSEX:
      snprintf(line, sizeof(line), "in sysex t=%u port=%d bytes=%u",
               record.sample_offset, record.port_index, record.size);
      break;
    case YLC_INPUT_EVENT_MIDI2:
      snprintf(line, sizeof(line), "in midi2 t=%u port=%d word0=0x%08x",
               record.sample_offset, record.port_index, record.size);
      break;
    case YLC_INPUT_EVENT_UNKNOWN:
    default:
      snprintf(line, sizeof(line), "in unknown t=%u type=%u size=%u",
               record.sample_offset, record.clap_type, record.size);
      break;
    }

    ylc_debug_log(self, "%s", line);
    read_seq++;
  }

  atomic_store_explicit(&self->input_event_log_read_seq, read_seq,
                        memory_order_release);
}

static double ylc_clamp_double(double value, double min_value,
                               double max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static void ylc_publish_program(ylc_plugin_t *self, ylc_program_t *program) {
  atomic_store_explicit(&self->active_program, program, memory_order_release);
}

static ylc_program_t *ylc_load_program(ylc_plugin_t *self) {
  return atomic_load_explicit(&self->active_program, memory_order_acquire);
}

static void ylc_clear_process_outputs(const clap_process_t *process) {
  if (!process || !process->audio_outputs || process->frames_count == 0) {
    return;
  }

  for (uint32_t port = 0; port < process->audio_outputs_count; ++port) {
    clap_audio_buffer_t *out = &process->audio_outputs[port];
    for (uint32_t ch = 0; ch < out->channel_count; ++ch) {
      if (out->data64 && out->data64[ch]) {
        memset(out->data64[ch], 0,
               sizeof(double) * (size_t)process->frames_count);
      }
      if (out->data32 && out->data32[ch]) {
        memset(out->data32[ch], 0,
               sizeof(float) * (size_t)process->frames_count);
      }
      if (ch < 64) {
        out->constant_mask |= ((uint64_t)1u << ch);
      }
    }
  }
}

static void ylc_begin_compile_barrier(ylc_plugin_t *self) {
  if (!self || !self->processing) {
    return;
  }

  atomic_store_explicit(&self->compile_in_progress, true, memory_order_release);
  while (atomic_load_explicit(&self->active_process_count,
                              memory_order_acquire) != 0) {
    usleep(100);
  }
}

static void ylc_end_compile_barrier(ylc_plugin_t *self) {
  if (!self) {
    return;
  }
  atomic_store_explicit(&self->compile_in_progress, false,
                        memory_order_release);
}

static bool ylc_transport_is_playing(const clap_event_transport_t *transport) {
  return transport && (transport->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
}

static double ylc_transport_tempo_bpm(const clap_event_transport_t *transport,
                                      double fallback) {
  if (transport && (transport->flags & CLAP_TRANSPORT_HAS_TEMPO) &&
      transport->tempo > 0.0) {
    return transport->tempo;
  }
  return fallback > 0.0 ? fallback : 120.0;
}

static bool ylc_audio_graph_transport_playing(void *plugin_state) {
  ylc_plugin_t *self = (ylc_plugin_t *)plugin_state;
  return self && self->transport_playing;
}

static double ylc_audio_graph_plugin_sample_rate(void *plugin_state) {
  ylc_plugin_t *self = (ylc_plugin_t *)plugin_state;
  return self && self->sample_rate > 0.0 ? self->sample_rate : 48000.0;
}

double ylc_plugin_param_value(int32_t index) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self || index < 0 || index >= (int32_t)YLC_PARAM_COUNT) {
    return 0.0;
  }
  return self->param_values[index];
}

double ylc_plugin_tempo_mul(void) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  const double bpm = self && self->tempo_bpm > 0.0 ? self->tempo_bpm : 120.0;
  return 60.0 / bpm;
}

static ylc_persistent_array_slot_t *
ylc_persistent_array_find(ylc_plugin_t *self, uint64_t key) {
  if (!self) {
    return NULL;
  }
  for (uint32_t i = 0; i < self->persistent_array_count; ++i) {
    if (self->persistent_arrays[i].key == key) {
      return &self->persistent_arrays[i];
    }
  }
  return NULL;
}

static double *ylc_persistent_array_alloc_values(uint32_t count) {
  if (count == 0) {
    return NULL;
  }
  const size_t count_size = (size_t)count;
  if (count > YLC_PERSIST_ARRAY_MAX_COUNT ||
      count_size >
          (SIZE_MAX - sizeof(ylc_persistent_array_rc_header_t)) /
              sizeof(double)) {
    return NULL;
  }

  ylc_persistent_array_rc_header_t *header =
      (ylc_persistent_array_rc_header_t *)calloc(
          1, sizeof(*header) + sizeof(double) * count_size);
  if (!header) {
    return NULL;
  }
  header->rc = 0;
  header->tag_or_size_class = count;
  return (double *)(header + 1);
}

static void ylc_persistent_array_free_values(double *values) {
  if (values) {
    free(((ylc_persistent_array_rc_header_t *)values) - 1);
  }
}

static ylc_persistent_array_slot_t *
ylc_persistent_array_create(ylc_plugin_t *self, uint64_t key) {
  if (!self ||
      self->persistent_array_count >= YLC_PERSIST_ARRAY_MAX_SLOTS) {
    return NULL;
  }

  if (self->persistent_array_count >= self->persistent_array_capacity) {
    uint32_t next_capacity = self->persistent_array_capacity > 0
                                 ? self->persistent_array_capacity * 2
                                 : 8;
    if (next_capacity > YLC_PERSIST_ARRAY_MAX_SLOTS) {
      next_capacity = YLC_PERSIST_ARRAY_MAX_SLOTS;
    }
    ylc_persistent_array_slot_t *next =
        (ylc_persistent_array_slot_t *)realloc(
            self->persistent_arrays, sizeof(*next) * next_capacity);
    if (!next) {
      return NULL;
    }
    self->persistent_arrays = next;
    self->persistent_array_capacity = next_capacity;
  }

  ylc_persistent_array_slot_t *slot =
      &self->persistent_arrays[self->persistent_array_count++];
  memset(slot, 0, sizeof(*slot));
  slot->key = key;
  return slot;
}

static bool ylc_persistent_array_set_count(ylc_persistent_array_slot_t *slot,
                                           _DoubleArray defaults) {
  if (!slot || defaults.size < 0 ||
      (uint32_t)defaults.size > YLC_PERSIST_ARRAY_MAX_COUNT) {
    return false;
  }

  uint32_t count = (uint32_t)defaults.size;
  if (slot->values && slot->count == count) {
    return true;
  }

  double *next = NULL;
  if (count > 0) {
    next = ylc_persistent_array_alloc_values(count);
    if (!next) {
      return false;
    }

    if (defaults.data) {
      for (uint32_t i = 0; i < count; ++i) {
        next[i] = defaults.data[i];
      }
    }

    if (slot->values) {
      uint32_t copy_count = slot->count < count ? slot->count : count;
      memcpy(next, slot->values, sizeof(double) * copy_count);
    }
  }

  ylc_persistent_array_free_values(slot->values);
  slot->values = next;
  slot->count = count;
  return true;
}

_DoubleArray ylc_plugin_persist_array(uint64_t key, _DoubleArray defaults) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return defaults;
  }

  ylc_persistent_array_slot_t *slot = ylc_persistent_array_find(self, key);
  if (!slot) {
    slot = ylc_persistent_array_create(self, key);
  }
  if (!slot || !ylc_persistent_array_set_count(slot, defaults)) {
    return defaults;
  }

  ylc_mark_state_dirty(self);
  return (_DoubleArray){
      .size = (int32_t)slot->count,
      .offset = 0,
      .data = slot->values,
  };
}

static void ylc_plugin_array_ui(uint32_t kind, const char *name,
                                _DoubleArray values) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self || values.size <= 0 || !values.data) {
    return;
  }

  double *data = values.data + values.offset;
  uint32_t count = (uint32_t)values.size;
  for (uint32_t i = 0; i < self->ui_count; ++i) {
    if (self->ui_slots[i].kind == (ylc_ui_kind_t)kind &&
        self->ui_slots[i].array_values == data &&
        self->ui_slots[i].array_count == count) {
      return;
    }
  }

  if (self->ui_count >= YLC_UI_MAX_SLOTS) {
    ylc_debug_log(self, "%s ignored: too many UI slots",
                  name ? name : "ArrayUI");
    return;
  }

  self->ui_slots[self->ui_count++] = (ylc_ui_slot_t){
      .kind = (ylc_ui_kind_t)kind, .array_count = count, .array_values = data};
  ylc_debug_log(self, "%s registered %u values",
                name ? name : "ArrayUI", count);
  if (self->gui_selected_array < 0) {
    self->gui_selected_array = 0;
  }
  ylc_gui_draw(self);
}

void EnvArrayUI(_DoubleArray values) {
  ylc_plugin_array_ui(YLC_UI_ENV, "EnvArrayUI", values);
}

void ADSRArrayUI(_DoubleArray values) {
  ylc_plugin_array_ui(YLC_UI_ADSR, "ADSRArrayUI", values);
}

int ctx_sample_rate(void) {
  return ylc_debug_printf_context && ylc_debug_printf_context->sample_rate > 0.0
             ? (int)ylc_debug_printf_context->sample_rate
             : 48000;
}

void ylc_plugin_install_dummy_jit_program(void *plugin_state) {
  ylc_plugin_t *self = (ylc_plugin_t *)plugin_state;
  if (!self) {
    return;
  }

  if (self->jit_dummy_graph.host_state != self) {
    ylc_dummy_audio_graph_init(
        &self->jit_dummy_graph, self, 0x12345678u ^ self->instance_id,
        ylc_audio_graph_transport_playing, ylc_audio_graph_plugin_sample_rate);
  }
  ylc_publish_program(self,
                      ylc_dummy_audio_graph_program(&self->jit_dummy_graph));
}

void ylc_plugin_prepare_script_audio_graph(void *plugin_state) {
  ylc_plugin_t *self = (ylc_plugin_t *)plugin_state;
  if (!self) {
    return;
  }

  atomic_store_explicit(&self->midi_in_handler, (uintptr_t)0,
                        memory_order_release);
  atomic_store_explicit(&self->param_in_handler, (uintptr_t)0,
                        memory_order_release);
  atomic_store_explicit(&self->param_mod_in_handler, (uintptr_t)0,
                        memory_order_release);
  atomic_store_explicit(&self->param_gesture_in_handler, (uintptr_t)0,
                        memory_order_release);

  if (!self->sf_inherit_from_state) {
    free(self->sf_inherit);
    self->sf_inherit = NULL;
    self->sf_inherit_count = 0;
    uint32_t sf_ui_count = 0;
    for (uint32_t i = 0; i < self->ui_count; ++i) {
      if (self->ui_slots[i].kind == YLC_UI_SOUNDFILE &&
          self->ui_slots[i].soundfile) {
        ++sf_ui_count;
      }
    }
    if (sf_ui_count > 0) {
      self->sf_inherit = (ylc_soundfile_inherit_t *)calloc(
          sf_ui_count, sizeof(ylc_soundfile_inherit_t));
      if (self->sf_inherit) {
        uint32_t idx = 0;
        for (uint32_t i = 0; i < self->ui_count; ++i) {
          if (self->ui_slots[i].kind != YLC_UI_SOUNDFILE ||
              !self->ui_slots[i].soundfile) {
            continue;
          }
          ylc_soundfile_t *sf = self->ui_slots[i].soundfile;
          snprintf(self->sf_inherit[idx].path,
                   sizeof(self->sf_inherit[idx].path), "%s", sf->user_path);
          self->sf_inherit[idx].region_start = sf->region_start;
          self->sf_inherit[idx].region_end = sf->region_end;
          ++idx;
        }
        self->sf_inherit_count = sf_ui_count;
      }
    }
  }
  self->sf_inherit_from_state = false;
  self->sf_inherit_index = 0;

  ylc_soundfile_free_all(self);
  self->ui_count = 0;
  self->gui_selected_array = -1;
  self->gui_selected_point = -1;
  self->gui_dragging = false;
  self->sf_dragging_edge = -1;

  ylc_audio_graph_clear(&self->jit_dummy_graph);
  ylc_clap_scheduler_clear(&self->scheduler);
}

void ylc_plugin_set_active_audio_graph(void *plugin_state) {
  ylc_plugin_t *self = (ylc_plugin_t *)plugin_state;
  if (!self) {
    return;
  }
  ylc_audio_graph_set_active_graph(&self->jit_dummy_graph);
  ylc_clap_scheduler_set_active(&self->scheduler);
}

void ylc_plugin_clear_active_audio_graph(void *plugin_state) {
  (void)plugin_state;
  ylc_audio_graph_set_active_graph(NULL);
  ylc_clap_scheduler_set_active(NULL);
}

void ylc_plugin_register_midi_in_handler(void *handler) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return;
  }

  atomic_store_explicit(&self->midi_in_handler, (uintptr_t)handler,
                        memory_order_release);
  ylc_debug_log(self, "registered MidiIn handler %p", handler);
}

void ylc_plugin_register_param_in_handler(void *handler) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return;
  }

  atomic_store_explicit(&self->param_in_handler, (uintptr_t)handler,
                        memory_order_release);
  ylc_debug_log(self, "registered ParamIn handler %p", handler);
}

void ylc_plugin_register_param_mod_in_handler(void *handler) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return;
  }

  atomic_store_explicit(&self->param_mod_in_handler, (uintptr_t)handler,
                        memory_order_release);
  ylc_debug_log(self, "registered ParamModIn handler %p", handler);
}

void ylc_plugin_register_param_gesture_in_handler(void *handler) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return;
  }

  atomic_store_explicit(&self->param_gesture_in_handler, (uintptr_t)handler,
                        memory_order_release);
  ylc_debug_log(self, "registered ParamGestureIn handler %p", handler);
}

void *ylc_plugin_audio_play_node(void *node) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self || !node) {
    return node;
  }

  ylc_audio_graph_play_node(&self->jit_dummy_graph, (Node *)node);
  ylc_publish_program(self,
                      ylc_dummy_audio_graph_program(&self->jit_dummy_graph));
  ylc_debug_log(self, "playing node %p", node);
  return node;
}

void *ylc_plugin_audio_reset_node(void *node) {
  if (!node) {
    return node;
  }

  return ylc_audio_graph_reset_node((Node *)node);
}

void *ylc_plugin_audio_play_voice(void *node) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self || !node) {
    return node;
  }

  ylc_audio_graph_play_voice(&self->jit_dummy_graph, (Node *)node);
  ylc_publish_program(self,
                      ylc_dummy_audio_graph_program(&self->jit_dummy_graph));
  return node;
}

void *ylc_plugin_audio_set_voice_input(int32_t input, double value,
                                       void *node) {
  if (!node) {
    return node;
  }

  return ylc_audio_graph_set_input_scalar((Node *)node, input, value);
}

static void ylc_mark_script_program_stale(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  self->script_program_ready = false;
  self->compiled_script_path[0] = '\0';
}

static bool ylc_script_program_is_current(ylc_plugin_t *self) {
  if (!self || !self->script_program_ready || self->script_path[0] == '\0') {
    return false;
  }

  if (atomic_load_explicit(&self->script_reload_pending,
                           memory_order_acquire)) {
    return false;
  }

  return strcmp(self->compiled_script_path, self->script_path) == 0;
}

static bool ylc_compile_and_install_script_program(ylc_plugin_t *self,
                                                    const char *reason) {
  if (!self || !self->runtime_service) {
    return false;
  }

  ylc_begin_compile_barrier(self);
  ylc_clap_scheduler_clear(&self->scheduler);
  char error[256] = {0};
  const bool ok = ylc_runtime_service_compile_script_program(
      self->runtime_service, self, self->script_path, error, sizeof(error),
      ylc_debug_compile_log, self);
  ylc_end_compile_barrier(self);
  if (ok) {
    self->script_program_ready = true;
    snprintf(self->compiled_script_path, sizeof(self->compiled_script_path),
             "%s", self->script_path);
    ylc_debug_log(self, "script JIT program compiled%s%s",
                  reason && reason[0] ? ": " : "",
                  reason && reason[0] ? reason : "");
  } else {
    ylc_mark_script_program_stale(self);
    ylc_debug_log(self, "script JIT compile failed%s%s%s",
                  reason && reason[0] ? " (" : "",
                  reason && reason[0] ? reason : "",
                  reason && reason[0] ? ")" : "");
    if (error[0] != '\0') {
      ylc_debug_log(self, "%s", error);
    }
  }
  return ok;
}

static bool ylc_reload_pending_script(ylc_plugin_t *self, const char *reason) {
  if (!self) {
    return false;
  }

  if (!atomic_exchange_explicit(&self->script_reload_pending, false,
                                memory_order_acq_rel)) {
    return false;
  }

  (void)ylc_compile_and_install_script_program(self, reason);
  ylc_gui_draw(self);
  return true;
}

static bool ylc_param_index_from_id(clap_id param_id, uint32_t *index) {
  if (param_id < YLC_PARAM_BASE_ID ||
      param_id >= YLC_PARAM_BASE_ID + YLC_PARAM_COUNT) {
    return false;
  }

  if (index) {
    *index = param_id - YLC_PARAM_BASE_ID;
  }
  return true;
}

static int32_t ylc_param_index_i32(clap_id param_id) {
  uint32_t index = 0;
  return ylc_param_index_from_id(param_id, &index) ? (int32_t)index : -1;
}

static int32_t ylc_param_id_i32(clap_id param_id) {
  return param_id <= (clap_id)INT32_MAX ? (int32_t)param_id : -1;
}

void ylc_mark_state_dirty(ylc_plugin_t *self) {
  if (self && self->host_state && self->host_state->mark_dirty) {
    self->host_state->mark_dirty(self->host);
  }
}

static bool ylc_stream_write_all(const clap_ostream_t *stream, const void *data,
                                 uint64_t size) {
  const uint8_t *cursor = (const uint8_t *)data;
  uint64_t remaining = size;

  while (remaining > 0) {
    const int64_t written = stream->write(stream, cursor, remaining);
    if (written <= 0) {
      return false;
    }

    cursor += written;
    remaining -= (uint64_t)written;
  }

  return true;
}

static bool ylc_stream_read_all(const clap_istream_t *stream, void *data,
                                uint64_t size) {
  uint8_t *cursor = (uint8_t *)data;
  uint64_t remaining = size;

  while (remaining > 0) {
    const int64_t read = stream->read(stream, cursor, remaining);
    if (read <= 0) {
      return false;
    }

    cursor += read;
    remaining -= (uint64_t)read;
  }

  return true;
}

static bool ylc_stream_read_optional_all(const clap_istream_t *stream,
                                         void *data, uint64_t size,
                                         bool *present) {
  if (present) {
    *present = false;
  }
  if (size == 0) {
    if (present) {
      *present = true;
    }
    return true;
  }

  uint8_t *cursor = (uint8_t *)data;
  const int64_t first = stream->read(stream, cursor, size);
  if (first == 0) {
    return true;
  }
  if (first < 0) {
    return false;
  }
  if (present) {
    *present = true;
  }

  cursor += first;
  uint64_t remaining = size - (uint64_t)first;
  while (remaining > 0) {
    const int64_t read = stream->read(stream, cursor, remaining);
    if (read <= 0) {
      return false;
    }
    cursor += read;
    remaining -= (uint64_t)read;
  }

  return true;
}

static void ylc_close_script_watcher(ylc_plugin_t *self) {
  if (!self || self->inotify_fd < 0) {
    return;
  }

  const int fd = self->inotify_fd;
  const bool registered = self->inotify_registered;
  char watched_name[YLC_SCRIPT_PATH_SIZE] = {0};
  snprintf(watched_name, sizeof(watched_name), "%s", self->watched_name);

  self->inotify_fd = -1;
  self->inotify_wd = -1;
  self->inotify_registered = false;
  self->watched_dir[0] = '\0';
  self->watched_name[0] = '\0';

  if (registered && self->host_posix_fd && self->host_posix_fd->unregister_fd) {
    self->host_posix_fd->unregister_fd(self->host, fd);
  }

  if (!self->destroying) {
    ylc_debug_log(self, "closed watcher for %s", watched_name);
  }
  close(fd);
}

static bool ylc_split_script_path(const char *path, char *dir, size_t dir_size,
                                  char *name, size_t name_size) {
  if (!path || path[0] == '\0' || !dir || dir_size == 0 || !name ||
      name_size == 0) {
    return false;
  }

  const char *slash = strrchr(path, '/');
  if (!slash) {
    snprintf(dir, dir_size, ".");
    snprintf(name, name_size, "%s", path);
    return name[0] != '\0';
  }

  if (slash == path) {
    snprintf(dir, dir_size, "/");
  } else {
    const size_t dir_len = (size_t)(slash - path);
    if (dir_len >= dir_size) {
      return false;
    }
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';
  }

  snprintf(name, name_size, "%s", slash + 1);
  return name[0] != '\0';
}

static bool ylc_string_has_suffix(const char *value, const char *suffix) {
  if (!value || !suffix) {
    return false;
  }

  const size_t value_len = strlen(value);
  const size_t suffix_len = strlen(suffix);
  return value_len >= suffix_len &&
         strcmp(value + value_len - suffix_len, suffix) == 0;
}

static void ylc_note_script_changed(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  ylc_mark_script_program_stale(self);
  atomic_store_explicit(&self->script_reload_pending, true,
                        memory_order_release);
  if (!self->destroying && self->clap_initialized && self->host &&
      self->host->request_callback) {
    self->host->request_callback(self->host);
  }
}

static void ylc_drain_script_watcher(ylc_plugin_t *self) {
  if (!self || self->inotify_fd < 0) {
    return;
  }

  union {
    struct inotify_event event;
    char buffer[4096];
  } events;

  for (;;) {
    const ssize_t bytes =
        read(self->inotify_fd, events.buffer, sizeof(events.buffer));
    if (bytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return;
      }
      ylc_debug_log(self, "watcher read failed: %s", strerror(errno));
      ylc_close_script_watcher(self);
      return;
    }
    if (bytes == 0) {
      return;
    }

    const char *cursor = events.buffer;
    const char *end = events.buffer + bytes;
    while (cursor < end) {
      const struct inotify_event *event = (const struct inotify_event *)cursor;
      const bool watched_file = event->len > 0 &&
                                (strcmp(event->name, self->watched_name) == 0 ||
                                 ylc_string_has_suffix(event->name, ".ylc"));
      const bool saved = (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) != 0;
      const bool watch_invalid =
          (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED)) != 0;

      if (watched_file && saved) {
        ylc_debug_log(self, "detected save for %s", event->name);
        ylc_note_script_changed(self);
      }
      if (watch_invalid) {
        ylc_debug_log(self, "watcher invalidated");
        ylc_close_script_watcher(self);
        return;
      }

      cursor += sizeof(struct inotify_event) + event->len;
    }
  }
}

void ylc_setup_script_watcher(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  char dir[YLC_SCRIPT_PATH_SIZE] = {0};
  char name[YLC_SCRIPT_PATH_SIZE] = {0};
  if (!ylc_split_script_path(self->script_path, dir, sizeof(dir), name,
                             sizeof(name))) {
    ylc_close_script_watcher(self);
    ylc_debug_log(self, "watch disabled: invalid script path");
    return;
  }

  if (self->inotify_fd >= 0 && strcmp(dir, self->watched_dir) == 0 &&
      strcmp(name, self->watched_name) == 0) {
    return;
  }

  ylc_close_script_watcher(self);

  const int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (fd < 0) {
    ylc_debug_log(self, "inotify init failed: %s", strerror(errno));
    return;
  }

  const uint32_t mask =
      IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF;
  const int wd = inotify_add_watch(fd, dir, mask);
  if (wd < 0) {
    ylc_debug_log(self, "cannot watch %s: %s", dir, strerror(errno));
    close(fd);
    return;
  }

  self->inotify_fd = fd;
  self->inotify_wd = wd;
  snprintf(self->watched_dir, sizeof(self->watched_dir), "%s", dir);
  snprintf(self->watched_name, sizeof(self->watched_name), "%s", name);
  atomic_store_explicit(&self->script_reload_pending, false,
                        memory_order_release);

  if (self->host_posix_fd && self->host_posix_fd->register_fd) {
    self->inotify_registered = self->host_posix_fd->register_fd(
        self->host, self->inotify_fd, CLAP_POSIX_FD_READ);
  }

  ylc_debug_log(self, "watching %s/%s%s", self->watched_dir, self->watched_name,
                self->inotify_registered ? " via host fd" : " via fallback");

  if (!self->destroying && self->clap_initialized &&
      !self->inotify_registered && self->host && self->host->request_callback) {
    self->host->request_callback(self->host);
  }
}

static void ylc_set_default_script_path(ylc_plugin_t *self) {
  const char *path = getenv("YLC_SCRIPT_PATH");
  if (path && path[0] != '\0') {
    snprintf(self->script_path, sizeof(self->script_path), "%s", path);
    return;
  }

  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    snprintf(self->script_path, sizeof(self->script_path),
             "%s/.config/ylc_clap/script.ylc", home);
    return;
  }

  snprintf(self->script_path, sizeof(self->script_path),
           "/tmp/ylc_clap-script.ylc");
}

void ylc_spawn_editor(ylc_plugin_t *self) {
  const char *terminal = getenv("YLC_TERMINAL");
  const char *editor = getenv("YLC_EDITOR");

  if (!terminal || terminal[0] == '\0') {
    terminal = "kitty";
  }
  if (!editor || editor[0] == '\0') {
    editor = "nvim";
  }

  pid_t child = fork();
  if (child < 0) {
    ylc_debug_log(self, "failed to fork editor launcher: %s", strerror(errno));
    return;
  }

  ylc_debug_log(self, "opening %s with %s in %s", self->script_path, editor,
                terminal);

  if (child == 0) {
    pid_t grandchild = fork();
    if (grandchild < 0) {
      _exit(127);
    }

    if (grandchild == 0) {
      setsid();
      char *const argv[] = {
          (char *)terminal,          "-e", (char *)editor,
          (char *)self->script_path, NULL,
      };
      execvp(terminal, argv);
      _exit(127);
    }

    _exit(0);
  }

  int status = 0;
  while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
  }
}

void ylc_spawn_log_follower(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  if (self->debug_log_path[0] == '\0') {
    ylc_debug_log(self, "cannot follow log: YLC_DEBUG_LOG is not set");
    return;
  }

  const char *terminal = getenv("YLC_TERMINAL");
  const char *follower = getenv("YLC_LOG_FOLLOWER");

  if (!terminal || terminal[0] == '\0') {
    terminal = "kitty";
  }
  if (!follower || follower[0] == '\0') {
    follower = "tail";
  }

  pid_t child = fork();
  if (child < 0) {
    ylc_debug_log(self, "failed to fork log follower: %s", strerror(errno));
    return;
  }

  ylc_debug_log(self, "following %s with %s -f in %s", self->debug_log_path,
                follower, terminal);

  if (child == 0) {
    pid_t grandchild = fork();
    if (grandchild < 0) {
      _exit(127);
    }

    if (grandchild == 0) {
      setsid();
      char *const argv[] = {
          (char *)terminal,     "-e", (char *)follower, "-f",
          self->debug_log_path, NULL,
      };
      execvp(terminal, argv);
      _exit(127);
    }

    _exit(0);
  }

  int status = 0;
  while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
  }
}

static bool ylc_init(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (self && self->host && self->host->get_extension) {
    self->clap_initialized = true;
    self->host_state = (const clap_host_state_t *)self->host->get_extension(
        self->host, CLAP_EXT_STATE);
    self->host_posix_fd =
        (const clap_host_posix_fd_support_t *)self->host->get_extension(
            self->host, CLAP_EXT_POSIX_FD_SUPPORT);
    self->reaper = (const reaper_plugin_info_t *)self->host->get_extension(
        self->host, YLC_REAPER_EXTENSION_ID);
    if (self->reaper && self->reaper->GetFunc) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
      self->clap_get_reaper_context =
          (ylc_clap_get_reaper_context_fn)self->reaper->GetFunc(
              "clap_get_reaper_context");
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
      if (self->clap_get_reaper_context) {
        self->reaper_parent_track =
            self->clap_get_reaper_context(self->host, 1);
        self->reaper_project = self->clap_get_reaper_context(self->host, 3);
      }
    }
    ylc_register_debug_pipe(self);
    ylc_debug_log(self, "debug pipe ready%s",
                  self->debug_pipe_registered ? " via host fd"
                                              : " via fallback");
    ylc_debug_log(self, "REAPER extension %s",
                  self->reaper ? "available" : "unavailable");
    if (self->clap_get_reaper_context) {
      ylc_debug_log(self, "REAPER context track=%p project=%p",
                    self->reaper_parent_track, self->reaper_project);
    }
    ylc_setup_script_watcher(self);
    ylc_compile_and_install_script_program(self, "init");
  }
  return self != NULL && self->host != NULL;
}

static void ylc_destroy(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self) {
    return;
  }

  self->destroying = true;
  self->clap_initialized = false;

  ylc_close_script_watcher(self);
  ylc_close_debug_pipe(self);
  ylc_close_debug_log_file(self);
  ylc_gui_close(self);
  ylc_audio_graph_free_all_nodes(&self->jit_dummy_graph);
  ylc_clap_scheduler_destroy(&self->scheduler);
  ylc_soundfile_free_all(self);
  free(self->sf_inherit);
  self->sf_inherit = NULL;
  self->sf_inherit_count = 0;
  for (uint32_t i = 0; i < self->persistent_array_count; ++i) {
    ylc_persistent_array_free_values(self->persistent_arrays[i].values);
  }
  free(self->persistent_arrays);
  if (self->runtime_service) {
    ylc_runtime_service_release(self->runtime_service, self->instance_id);
    self->runtime_service = NULL;
    self->instance_id = 0;
  }
  free(self);
}

static bool ylc_activate(const clap_plugin_t *plugin, double sample_rate,
                         uint32_t min_frames_count, uint32_t max_frames_count) {
  (void)min_frames_count;

  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || sample_rate <= 0.0 || max_frames_count == 0) {
    return false;
  }

  self->sample_rate = sample_rate;
  self->max_frames_count = max_frames_count;
  self->scheduler.sample_rate = (int)sample_rate;
  return true;
}

static void ylc_deactivate(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self) {
    return;
  }

  self->sample_rate = 0.0;
  self->max_frames_count = 0;
}

static bool ylc_start_processing(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || self->sample_rate <= 0.0) {
    return false;
  }

  if (!ylc_script_program_is_current(self)) {
    ylc_compile_and_install_script_program(self, "process start");
  }
  self->processing = true;
  return true;
}

static void ylc_stop_processing(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (self) {
    self->processing = false;
    ylc_clap_scheduler_clear(&self->scheduler);
  }
}

static void ylc_reset(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (self) {
    for (uint32_t i = 0; i < YLC_PARAM_COUNT; ++i) {
      self->param_values[i] = 0.0;
    }
    self->param_values[YLC_PARAM_FIRST_INDEX] = 1.0;
    self->tempo_bpm = 120.0;
    ylc_clap_scheduler_clear(&self->scheduler);
  }
}

static void ylc_invoke_param_in_handler(ylc_plugin_t *self, int32_t param_index,
                                        int32_t param_id, double value) {
  if (!self) {
    return;
  }

  const uintptr_t raw =
      atomic_load_explicit(&self->param_in_handler, memory_order_acquire);
  if (!raw) {
    return;
  }

  ylc_param_in_handler_fn handler = (ylc_param_in_handler_fn)raw;

  ylc_plugin_debug_printf_set_context(self);
  handler(param_index, param_id, value);
  ylc_plugin_debug_printf_clear_context(self);
}

static void ylc_invoke_param_mod_in_handler(ylc_plugin_t *self,
                                            int32_t param_index,
                                            int32_t param_id, double amount) {
  if (!self) {
    return;
  }

  const uintptr_t raw =
      atomic_load_explicit(&self->param_mod_in_handler, memory_order_acquire);
  if (!raw) {
    return;
  }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
  ylc_param_mod_in_handler_fn handler = (ylc_param_mod_in_handler_fn)raw;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  ylc_plugin_debug_printf_set_context(self);
  handler(param_index, param_id, amount);
  ylc_plugin_debug_printf_clear_context(self);
}

static void ylc_invoke_param_gesture_in_handler(ylc_plugin_t *self,
                                                int32_t param_index,
                                                int32_t param_id,
                                                uint8_t gesture_type) {
  if (!self) {
    return;
  }

  const uintptr_t raw = atomic_load_explicit(&self->param_gesture_in_handler,
                                             memory_order_acquire);
  if (!raw) {
    return;
  }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
  ylc_param_gesture_in_handler_fn handler =
      (ylc_param_gesture_in_handler_fn)raw;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  ylc_plugin_debug_printf_set_context(self);
  handler(param_index, param_id, gesture_type);
  ylc_plugin_debug_printf_clear_context(self);
}

static void ylc_dispatch_event(ylc_plugin_t *self, ylc_program_t *program,
                               const clap_event_header_t *header) {
  if (!self || !program || !header ||
      header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
    return;
  }

  ylc_event_handlers_dispatch(&self->event_handlers, header);

  switch (header->type) {
  case CLAP_EVENT_NOTE_ON:
    ylc_runtime_note_on(program, header->time,
                        (const clap_event_note_t *)header);
    break;
  case CLAP_EVENT_NOTE_OFF:
    ylc_runtime_note_off(program, header->time,
                         (const clap_event_note_t *)header);
    break;
  case CLAP_EVENT_PARAM_VALUE: {
    const clap_event_param_value_t *event =
        (const clap_event_param_value_t *)header;
    uint32_t index = 0;
    int32_t param_index = -1;
    if (ylc_param_index_from_id(event->param_id, &index)) {
      param_index = (int32_t)index;
      self->param_values[index] = ylc_clamp_double(event->value, 0.0, 1.0);
    }
    ylc_runtime_param(program, header->time, event->param_id, event->value);
    ylc_invoke_param_in_handler(
        self, param_index, ylc_param_id_i32(event->param_id), event->value);
    break;
  }
  case CLAP_EVENT_PARAM_MOD: {
    const clap_event_param_mod_t *event =
        (const clap_event_param_mod_t *)header;
    ylc_invoke_param_mod_in_handler(self, ylc_param_index_i32(event->param_id),
                                    ylc_param_id_i32(event->param_id),
                                    event->amount);
    break;
  }
  case CLAP_EVENT_PARAM_GESTURE_BEGIN:
  case CLAP_EVENT_PARAM_GESTURE_END: {
    const clap_event_param_gesture_t *event =
        (const clap_event_param_gesture_t *)header;
    const uint8_t gesture_type = header->type == CLAP_EVENT_PARAM_GESTURE_BEGIN
                                     ? YLC_PARAM_GESTURE_TYPE_BEGIN
                                     : YLC_PARAM_GESTURE_TYPE_END;
    ylc_invoke_param_gesture_in_handler(
        self, ylc_param_index_i32(event->param_id),
        ylc_param_id_i32(event->param_id), gesture_type);
    break;
  }
  case CLAP_EVENT_MIDI:
    ylc_runtime_midi(program, header->time, (const clap_event_midi_t *)header);
    break;
  case CLAP_EVENT_TRANSPORT:
    self->transport_playing =
        ylc_transport_is_playing((const clap_event_transport_t *)header);
    self->tempo_bpm =
        ylc_transport_tempo_bpm((const clap_event_transport_t *)header,
                                self->tempo_bpm);
    ylc_runtime_transport(program, header->time,
                          (const clap_event_transport_t *)header);
    break;
  default:
    break;
  }
}

static void ylc_invoke_midi_in_handler(ylc_plugin_t *self, int32_t channel,
                                       uint8_t event_type, int32_t note,
                                       double value) {
  if (!self) {
    return;
  }

  const uintptr_t raw =
      atomic_load_explicit(&self->midi_in_handler, memory_order_acquire);
  if (!raw) {
    return;
  }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
  ylc_midi_in_handler_fn handler = (ylc_midi_in_handler_fn)raw;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  ylc_plugin_debug_printf_set_context(self);
  handler(channel, event_type, note, value);
  ylc_plugin_debug_printf_clear_context(self);
}

static void ylc_dispatch_script_midi_in(ylc_plugin_t *self,
                                        const clap_event_header_t *header) {
  if (!self || !header || header->space_id != CLAP_CORE_EVENT_SPACE_ID) {
    return;
  }

  switch (header->type) {
  case CLAP_EVENT_NOTE_ON: {
    const clap_event_note_t *event = (const clap_event_note_t *)header;
    ylc_invoke_midi_in_handler(self, event->channel,
                               YLC_MIDI_EVENT_TYPE_NOTE_ON, event->key,
                               event->velocity);
    break;
  }
  case CLAP_EVENT_NOTE_OFF: {
    const clap_event_note_t *event = (const clap_event_note_t *)header;
    ylc_invoke_midi_in_handler(self, event->channel,
                               YLC_MIDI_EVENT_TYPE_NOTE_OFF, event->key,
                               event->velocity);
    break;
  }
  case CLAP_EVENT_MIDI: {
    const clap_event_midi_t *event = (const clap_event_midi_t *)header;
    const uint8_t status = event->data[0] & 0xf0u;
    const int32_t channel = (int32_t)(event->data[0] & 0x0fu);
    const int32_t data1 = (int32_t)event->data[1];
    const int32_t data2 = (int32_t)event->data[2];
    const double value = (double)data2 / 127.0;

    if (status == 0x90u && data2 > 0) {
      ylc_invoke_midi_in_handler(self, channel, YLC_MIDI_EVENT_TYPE_NOTE_ON,
                                 data1, value);
    } else if (status == 0x80u || status == 0x90u) {
      ylc_invoke_midi_in_handler(self, channel, YLC_MIDI_EVENT_TYPE_NOTE_OFF,
                                 data1, value);
    } else if (status == 0xb0u) {
      ylc_invoke_midi_in_handler(self, channel, YLC_MIDI_EVENT_TYPE_CC, data1,
                                 value);
    }
    break;
  }
  default:
    break;
  }
}

static void ylc_process_audio_span_with_context(ylc_plugin_t *self,
                                                ylc_program_t *program,
                                                const clap_process_t *process,
                                                uint32_t offset,
                                                uint32_t frames_count) {
  if (!self || !program || !process || frames_count == 0) {
    return;
  }

  ylc_plugin_debug_printf_set_context(self);
  ylc_runtime_process_span(program, process, offset, frames_count,
                           self->param_values[YLC_PARAM_FIRST_INDEX]);
  ylc_plugin_debug_printf_clear_context(self);
}

static clap_process_status ylc_process(const clap_plugin_t *plugin,
                                       const clap_process_t *process) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !process) {
    return CLAP_PROCESS_ERROR;
  }

  if (atomic_load_explicit(&self->compile_in_progress, memory_order_acquire)) {
    ylc_clear_process_outputs(process);
    return CLAP_PROCESS_CONTINUE;
  }

  atomic_fetch_add_explicit(&self->active_process_count, 1,
                            memory_order_acq_rel);
  if (atomic_load_explicit(&self->compile_in_progress, memory_order_acquire)) {
    atomic_fetch_sub_explicit(&self->active_process_count, 1,
                              memory_order_acq_rel);
    ylc_clear_process_outputs(process);
    return CLAP_PROCESS_CONTINUE;
  }

  ylc_program_t *program = ylc_load_program(self);
  if (!program) {
    atomic_fetch_sub_explicit(&self->active_process_count, 1,
                              memory_order_acq_rel);
    return CLAP_PROCESS_ERROR;
  }

  const uint32_t frames_count = process->frames_count;
  self->transport_playing = ylc_transport_is_playing(process->transport);
  self->tempo_bpm = ylc_transport_tempo_bpm(process->transport, self->tempo_bpm);

  ylc_clap_scheduler_set_active(&self->scheduler);
  ylc_audio_graph_set_active_graph(&self->jit_dummy_graph);
  ylc_plugin_debug_printf_set_context(self);
  ylc_clap_scheduler_drain(&self->scheduler);
  ylc_plugin_debug_printf_clear_context(self);
  ylc_audio_graph_set_active_graph(NULL);

  uint32_t cursor = 0;
  const clap_input_events_t *events = process->in_events;
  const uint32_t event_count =
      events && events->size ? events->size(events) : 0;

  for (uint32_t i = 0; i < event_count; ++i) {
    if (!events->get) {
      break;
    }

    const clap_event_header_t *event = events->get(events, i);
    if (!event) {
      continue;
    }

    const uint32_t event_time =
        event->time <= frames_count ? event->time : frames_count;
    if (event_time > cursor) {
      ylc_process_audio_span_with_context(self, program, process, cursor,
                                          event_time - cursor);
      cursor = event_time;
    }

    ylc_audio_graph_set_active_graph(&self->jit_dummy_graph);
    ylc_dispatch_event(self, program, event);
    ylc_dispatch_script_midi_in(self, event);
    ylc_audio_graph_set_active_graph(NULL);
  }

  if (cursor < frames_count) {
    ylc_process_audio_span_with_context(self, program, process, cursor,
                                        frames_count - cursor);
  }

  ylc_clap_scheduler_advance(&self->scheduler, frames_count);
  ylc_clap_scheduler_set_active(NULL);

  atomic_fetch_sub_explicit(&self->active_process_count, 1,
                            memory_order_acq_rel);
  return CLAP_PROCESS_CONTINUE;
}

static uint32_t ylc_audio_ports_count(const clap_plugin_t *plugin,
                                      bool is_input) {
  (void)plugin;
  (void)is_input;
  return 1;
}

static bool ylc_audio_ports_get(const clap_plugin_t *plugin, uint32_t index,
                                bool is_input, clap_audio_port_info_t *info) {
  (void)plugin;

  if (!info || index != 0) {
    return false;
  }

  memset(info, 0, sizeof(*info));
  info->id = is_input ? 0 : 1;
  info->flags = CLAP_AUDIO_PORT_IS_MAIN | CLAP_AUDIO_PORT_SUPPORTS_64BITS |
                CLAP_AUDIO_PORT_PREFERS_64BITS |
                CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = is_input ? 1 : 0;
  strncpy(info->name, is_input ? "Stereo In" : "Stereo Out",
          sizeof(info->name) - 1);
  return true;
}

static uint32_t ylc_note_ports_count(const clap_plugin_t *plugin,
                                     bool is_input) {
  (void)plugin;
  return is_input ? 1 : 0;
}

static bool ylc_note_ports_get(const clap_plugin_t *plugin, uint32_t index,
                               bool is_input, clap_note_port_info_t *info) {
  (void)plugin;

  if (!info || !is_input || index != 0) {
    return false;
  }

  memset(info, 0, sizeof(*info));
  info->id = 0;
  info->supported_dialects =
      CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI2;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  strncpy(info->name, "Script Events In", sizeof(info->name) - 1);
  return true;
}

static uint32_t ylc_params_count(const clap_plugin_t *plugin) {
  (void)plugin;
  return YLC_PARAM_COUNT;
}

static bool ylc_params_get_info(const clap_plugin_t *plugin,
                                uint32_t param_index,
                                clap_param_info_t *param_info) {
  (void)plugin;

  if (!param_info || param_index >= YLC_PARAM_COUNT) {
    return false;
  }

  memset(param_info, 0, sizeof(*param_info));
  param_info->id = YLC_PARAM_BASE_ID + param_index;
  param_info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
  snprintf(param_info->name, sizeof(param_info->name), "Param %02u",
           param_index + 1);
  strncpy(param_info->module, "Script Params", sizeof(param_info->module) - 1);
  param_info->min_value = 0.0;
  param_info->max_value = 1.0;
  param_info->default_value = param_index == YLC_PARAM_FIRST_INDEX ? 1.0 : 0.0;
  return true;
}

static bool ylc_params_get_value(const clap_plugin_t *plugin, clap_id param_id,
                                 double *value) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !value) {
    return false;
  }

  uint32_t index = 0;
  if (!ylc_param_index_from_id(param_id, &index)) {
    return false;
  }

  *value = self->param_values[index];
  return true;
}

static bool ylc_params_value_to_text(const clap_plugin_t *plugin,
                                     clap_id param_id, double value,
                                     char *display, uint32_t size) {
  (void)plugin;
  if (!display || size == 0) {
    return false;
  }

  if (!ylc_param_index_from_id(param_id, NULL)) {
    return false;
  }

  const double pct = ylc_clamp_double(value, 0.0, 1.0) * 100.0;
  const int whole = (int)(pct + 0.5);
  int written = snprintf(display, size, "%d%%", whole);
  return written >= 0 && (uint32_t)written < size;
}

static bool ylc_params_text_to_value(const clap_plugin_t *plugin,
                                     clap_id param_id, const char *display,
                                     double *value) {
  (void)plugin;
  if (!display || !value) {
    return false;
  }

  if (!ylc_param_index_from_id(param_id, NULL)) {
    return false;
  }

  char *end = NULL;
  const double parsed = strtod(display, &end);
  if (end == display) {
    return false;
  }

  *value = ylc_clamp_double(parsed / 100.0, 0.0, 1.0);
  return true;
}

static void ylc_params_flush(const clap_plugin_t *plugin,
                             const clap_input_events_t *in,
                             const clap_output_events_t *out) {
  (void)out;

  ylc_plugin_t *self = ylc_from_plugin(plugin);
  ylc_program_t *program = self ? ylc_load_program(self) : NULL;
  const uint32_t event_count = in && in->size ? in->size(in) : 0;

  for (uint32_t i = 0; i < event_count; ++i) {
    const clap_event_header_t *event = in->get ? in->get(in, i) : NULL;
    if (event && event->space_id == CLAP_CORE_EVENT_SPACE_ID &&
        event->type == CLAP_EVENT_PARAM_VALUE) {
      ylc_dispatch_event(self, program, event);
    }
  }
}

static bool ylc_state_save(const clap_plugin_t *plugin,
                           const clap_ostream_t *stream) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !stream || !stream->write) {
    return false;
  }

  const uint32_t path_len = (uint32_t)strlen(self->script_path);
  ylc_saved_state_header_t header = {
      .magic = YLC_STATE_MAGIC,
      .version = YLC_STATE_VERSION,
      .script_path_len = path_len,
  };
  memcpy(header.param_values, self->param_values, sizeof(header.param_values));

  bool ok = ylc_stream_write_all(stream, &header, sizeof(header)) &&
            ylc_stream_write_all(stream, self->script_path, path_len);

  uint32_t slot_count = self->persistent_array_count;
  ok = ok && ylc_stream_write_all(stream, &slot_count, sizeof(slot_count));
  for (uint32_t i = 0; ok && i < slot_count; ++i) {
    ylc_persistent_array_slot_t *slot = &self->persistent_arrays[i];
    ok = ylc_stream_write_all(stream, &slot->key, sizeof(slot->key)) &&
         ylc_stream_write_all(stream, &slot->count, sizeof(slot->count));
    if (ok && slot->count > 0) {
      ok = slot->values &&
           ylc_stream_write_all(stream, slot->values,
                                sizeof(double) * slot->count);
    }
  }

  uint32_t sf_count = 0;
  for (uint32_t i = 0; i < self->ui_count; ++i) {
    if (self->ui_slots[i].kind == YLC_UI_SOUNDFILE &&
        self->ui_slots[i].soundfile) {
      ++sf_count;
    }
  }
  ok = ok && ylc_stream_write_all(stream, &sf_count, sizeof(sf_count));
  for (uint32_t i = 0; ok && i < self->ui_count; ++i) {
    if (self->ui_slots[i].kind != YLC_UI_SOUNDFILE ||
        !self->ui_slots[i].soundfile) {
      continue;
    }
    ylc_soundfile_t *sf = self->ui_slots[i].soundfile;
    uint64_t dummy_key = 0;
    ok = ylc_stream_write_all(stream, &dummy_key, sizeof(dummy_key));
    uint32_t path_len = (uint32_t)strlen(sf->user_path);
    ok = ok && ylc_stream_write_all(stream, &path_len, sizeof(path_len));
    if (ok && path_len > 0) {
      ok = ylc_stream_write_all(stream, sf->user_path, path_len);
    }
    uint64_t rs = sf->region_start;
    uint64_t re = sf->region_end;
    ok = ok && ylc_stream_write_all(stream, &rs, sizeof(rs)) &&
         ylc_stream_write_all(stream, &re, sizeof(re));
  }

  ylc_debug_log(self, "state save %s", ok ? "ok" : "failed");
  return ok;
}

static bool ylc_state_load(const clap_plugin_t *plugin,
                           const clap_istream_t *stream) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self || !stream || !stream->read) {
    return false;
  }

  ylc_saved_state_header_t header = {0};
  if (!ylc_stream_read_all(stream, &header, sizeof(header))) {
    ylc_debug_log(self, "state load failed: short header");
    return false;
  }

  if (header.magic != YLC_STATE_MAGIC || header.version != YLC_STATE_VERSION ||
      header.script_path_len >= sizeof(self->script_path)) {
    ylc_debug_log(self, "state load failed: incompatible state");
    return false;
  }

  for (uint32_t i = 0; i < YLC_PARAM_COUNT; ++i) {
    self->param_values[i] = ylc_clamp_double(header.param_values[i], 0.0, 1.0);
  }
  if (!ylc_stream_read_all(stream, self->script_path, header.script_path_len)) {
    ylc_debug_log(self, "state load failed: short path");
    return false;
  }

  self->script_path[header.script_path_len] = '\0';

  uint32_t slot_count = 0;
  bool has_persistent_arrays = false;
  if (!ylc_stream_read_optional_all(stream, &slot_count, sizeof(slot_count),
                                    &has_persistent_arrays)) {
    ylc_debug_log(self, "state load failed: malformed persistent array count");
    return false;
  }

  ylc_persistent_array_slot_t *loaded_slots = NULL;
  uint32_t loaded_count = 0;
  if (has_persistent_arrays) {
    if (slot_count > YLC_PERSIST_ARRAY_MAX_SLOTS) {
      ylc_debug_log(self, "state load failed: too many persistent arrays");
      return false;
    }
    if (slot_count > 0) {
      loaded_slots =
          (ylc_persistent_array_slot_t *)calloc(slot_count, sizeof(*loaded_slots));
      if (!loaded_slots) {
        return false;
      }
    }

    for (uint32_t i = 0; i < slot_count; ++i) {
      uint64_t key = 0;
      uint32_t count = 0;
      if (!ylc_stream_read_all(stream, &key, sizeof(key)) ||
          !ylc_stream_read_all(stream, &count, sizeof(count))) {
        ylc_debug_log(self, "state load failed: short persistent array header");
        for (uint32_t j = 0; j < loaded_count; ++j) {
          ylc_persistent_array_free_values(loaded_slots[j].values);
        }
        free(loaded_slots);
        return false;
      }
      if (count > YLC_PERSIST_ARRAY_MAX_COUNT) {
        ylc_debug_log(self, "state load failed: persistent array too large");
        for (uint32_t j = 0; j < loaded_count; ++j) {
          ylc_persistent_array_free_values(loaded_slots[j].values);
        }
        free(loaded_slots);
        return false;
      }

      loaded_slots[i].key = key;
      loaded_slots[i].count = count;
      if (count > 0) {
        loaded_slots[i].values = ylc_persistent_array_alloc_values(count);
        if (!loaded_slots[i].values ||
            !ylc_stream_read_all(stream, loaded_slots[i].values,
                                 sizeof(double) * count)) {
          ylc_debug_log(self, "state load failed: short persistent array data");
          for (uint32_t j = 0; j <= loaded_count; ++j) {
            ylc_persistent_array_free_values(loaded_slots[j].values);
          }
          free(loaded_slots);
          return false;
        }
      }
      loaded_count++;
    }
  }

  for (uint32_t i = 0; i < self->persistent_array_count; ++i) {
    ylc_persistent_array_free_values(self->persistent_arrays[i].values);
  }
  free(self->persistent_arrays);
  self->persistent_arrays = loaded_slots;
  self->persistent_array_count = loaded_count;
  self->persistent_array_capacity = loaded_count;

  uint32_t sf_count = 0;
  bool has_soundfiles = false;
  if (!ylc_stream_read_optional_all(stream, &sf_count, sizeof(sf_count),
                                    &has_soundfiles)) {
    ylc_debug_log(self, "state load failed: malformed soundfile count");
    return false;
  }
  free(self->sf_inherit);
  self->sf_inherit = NULL;
  self->sf_inherit_count = 0;
  self->sf_inherit_from_state = true;
  if (has_soundfiles && sf_count > 0 && sf_count <= YLC_SOUNDFILE_MAX_SLOTS) {
    self->sf_inherit = (ylc_soundfile_inherit_t *)calloc(
        sf_count, sizeof(ylc_soundfile_inherit_t));
    if (!self->sf_inherit) {
      return false;
    }
    for (uint32_t i = 0; i < sf_count; ++i) {
      ylc_soundfile_inherit_t *p = &self->sf_inherit[i];
      uint64_t key = 0;
      uint32_t path_len = 0;
      if (!ylc_stream_read_all(stream, &key, sizeof(key)) ||
          !ylc_stream_read_all(stream, &path_len, sizeof(path_len))) {
        free(self->sf_inherit);
        self->sf_inherit = NULL;
        return false;
      }
      (void)key;
      if (path_len >= YLC_SOUNDFILE_PATH_SIZE) {
        free(self->sf_inherit);
        self->sf_inherit = NULL;
        return false;
      }
      if (path_len > 0 &&
          !ylc_stream_read_all(stream, p->path, path_len)) {
        free(self->sf_inherit);
        self->sf_inherit = NULL;
        return false;
      }
      p->path[path_len] = '\0';
      if (!ylc_stream_read_all(stream, &p->region_start,
                               sizeof(p->region_start)) ||
          !ylc_stream_read_all(stream, &p->region_end,
                               sizeof(p->region_end))) {
        free(self->sf_inherit);
        self->sf_inherit = NULL;
        return false;
      }
      self->sf_inherit_count++;
    }
  }

  ylc_mark_script_program_stale(self);
  atomic_store_explicit(&self->script_reload_pending, false,
                        memory_order_release);
  ylc_setup_script_watcher(self);
  ylc_debug_log(self, "state load ok");
  ylc_gui_draw(self);
  return true;
}

static void ylc_posix_on_fd(const clap_plugin_t *plugin, int fd,
                            clap_posix_fd_flags_t flags) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self) {
    return;
  }

  if (fd == self->debug_pipe_read_fd &&
      (flags & (CLAP_POSIX_FD_READ | CLAP_POSIX_FD_ERROR)) != 0) {
    ylc_debug_drain_pipe(self);
    return;
  }

  if (fd == self->inotify_fd &&
      (flags & (CLAP_POSIX_FD_READ | CLAP_POSIX_FD_ERROR)) != 0) {
    ylc_drain_script_watcher(self);
    ylc_reload_pending_script(self, "script save");
    ylc_gui_draw(self);
  }
}

static const void *ylc_get_extension(const clap_plugin_t *plugin,
                                     const char *id) {
  (void)plugin;

  if (!id) {
    return NULL;
  }

  static const clap_plugin_audio_ports_t audio_ports = {
      .count = ylc_audio_ports_count,
      .get = ylc_audio_ports_get,
  };
  static const clap_plugin_note_ports_t note_ports = {
      .count = ylc_note_ports_count,
      .get = ylc_note_ports_get,
  };
  static const clap_plugin_params_t params = {
      .count = ylc_params_count,
      .get_info = ylc_params_get_info,
      .get_value = ylc_params_get_value,
      .value_to_text = ylc_params_value_to_text,
      .text_to_value = ylc_params_text_to_value,
      .flush = ylc_params_flush,
  };
  static const clap_plugin_state_t state = {
      .save = ylc_state_save,
      .load = ylc_state_load,
  };
  static const clap_plugin_posix_fd_support_t posix_fd = {
      .on_fd = ylc_posix_on_fd,
  };

  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) {
    return &audio_ports;
  }
  if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) {
    return &note_ports;
  }
  if (strcmp(id, CLAP_EXT_PARAMS) == 0) {
    return &params;
  }
  if (strcmp(id, CLAP_EXT_GUI) == 0) {
    return ylc_gui_extension();
  }
  if (strcmp(id, CLAP_EXT_STATE) == 0) {
    return &state;
  }
  if (strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT) == 0) {
    return &posix_fd;
  }
  return NULL;
}

static void ylc_on_main_thread(const clap_plugin_t *plugin) {
  ylc_plugin_t *self = ylc_from_plugin(plugin);
  if (!self) {
    return;
  }

  ylc_gui_poll_events(self);
  if (!self->debug_pipe_registered) {
    ylc_debug_drain_pipe(self);
  }
  ylc_drain_input_event_log(self);
  if (!self->inotify_registered) {
    ylc_drain_script_watcher(self);
  }

  ylc_reload_pending_script(self, "script save");

  if ((self->gui_visible ||
       (self->inotify_fd >= 0 && !self->inotify_registered)) &&
      !self->destroying && self->clap_initialized && self->host &&
      self->host->request_callback) {
    self->host->request_callback(self->host);
  }
}

static const char *const ylc_features[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_STEREO,
    NULL,
};

static const clap_plugin_descriptor_t ylc_descriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = YLC_PLUGIN_ID,
    .name = "YLC Script Shim",
    .vendor = "YLC",
    .url = "https://example.invalid/ylc_clap",
    .manual_url = "https://example.invalid/ylc_clap/manual",
    .support_url = "https://example.invalid/ylc_clap/support",
    .version = "0.1.0",
    .description = "Minimal CLAP shim for a future script/JIT runtime.",
    .features = ylc_features,
};

static uint32_t
ylc_factory_get_plugin_count(const clap_plugin_factory_t *factory) {
  (void)factory;
  return 1;
}

static const clap_plugin_descriptor_t *
ylc_factory_get_plugin_descriptor(const clap_plugin_factory_t *factory,
                                  uint32_t index) {
  (void)factory;
  return index == 0 ? &ylc_descriptor : NULL;
}

static const clap_plugin_t *
ylc_factory_create_plugin(const clap_plugin_factory_t *factory,
                          const clap_host_t *host, const char *plugin_id) {
  (void)factory;

  if (!host || !clap_version_is_compatible(host->clap_version) ||
      strcmp(plugin_id, YLC_PLUGIN_ID) != 0) {
    return NULL;
  }

  ylc_plugin_t *self = (ylc_plugin_t *)calloc(1, sizeof(*self));
  if (!self) {
    return NULL;
  }

  uint32_t instance_id = 0;
  ylc_runtime_service_t *runtime_service =
      ylc_runtime_service_acquire(&instance_id);
  if (!runtime_service) {
    free(self);
    return NULL;
  }

  self->host = host;
  self->runtime_service = runtime_service;
  self->instance_id = instance_id;
  for (uint32_t i = 0; i < YLC_PARAM_COUNT; ++i) {
    self->param_values[i] = 0.0;
  }
  self->param_values[YLC_PARAM_FIRST_INDEX] = 1.0;
  self->tempo_bpm = 120.0;
  self->inotify_fd = -1;
  self->inotify_wd = -1;
  self->debug_pipe_read_fd = -1;
  self->debug_pipe_write_fd = -1;
  atomic_init(&self->script_reload_pending, false);
  atomic_init(&self->input_event_log_write_seq, 0);
  atomic_init(&self->input_event_log_read_seq, 0);
  atomic_init(&self->input_event_log_dropped, 0);
  atomic_init(&self->midi_in_handler, (uintptr_t)0);
  atomic_init(&self->param_in_handler, (uintptr_t)0);
  atomic_init(&self->param_mod_in_handler, (uintptr_t)0);
  atomic_init(&self->param_gesture_in_handler, (uintptr_t)0);
  atomic_init(&self->active_process_count, 0);
  atomic_init(&self->compile_in_progress, false);
  ylc_open_debug_pipe(self);
  const char *debug_log_path = getenv("YLC_DEBUG_LOG");
  if (debug_log_path && debug_log_path[0] != '\0') {
    int open_errno = 0;
    if (ylc_open_debug_log_file(self, debug_log_path, &open_errno)) {
      ylc_debug_log(self, "debug log tee ready: %s", debug_log_path);
    } else {
      ylc_debug_log(self, "debug log tee failed: %s: %s", debug_log_path,
                    strerror(open_errno));
    }
  }
  ylc_debug_log(self, "runtime service acquired: instance=%u refs=%u",
                self->instance_id,
                ylc_runtime_service_ref_count(self->runtime_service));
  ylc_set_default_script_path(self);
  ylc_event_handlers_init(self);
  ylc_runtime_init_bypass_program(&self->fallback_program);
  ylc_dummy_audio_graph_init(
      &self->jit_dummy_graph, self, 0x12345678u ^ self->instance_id,
      ylc_audio_graph_transport_playing, ylc_audio_graph_plugin_sample_rate);
  ylc_clap_scheduler_init(&self->scheduler, (int)self->sample_rate);
  ylc_publish_program(self, &self->fallback_program);

  self->plugin.desc = &ylc_descriptor;
  self->plugin.plugin_data = self;
  self->plugin.init = ylc_init;
  self->plugin.destroy = ylc_destroy;
  self->plugin.activate = ylc_activate;
  self->plugin.deactivate = ylc_deactivate;
  self->plugin.start_processing = ylc_start_processing;
  self->plugin.stop_processing = ylc_stop_processing;
  self->plugin.reset = ylc_reset;
  self->plugin.process = ylc_process;
  self->plugin.get_extension = ylc_get_extension;
  self->plugin.on_main_thread = ylc_on_main_thread;

  return &self->plugin;
}

static const clap_plugin_factory_t ylc_factory = {
    .get_plugin_count = ylc_factory_get_plugin_count,
    .get_plugin_descriptor = ylc_factory_get_plugin_descriptor,
    .create_plugin = ylc_factory_create_plugin,
};

static bool ylc_entry_init(const char *plugin_path) {
  (void)plugin_path;
  return ylc_runtime_service_global_init();
}

static void ylc_entry_deinit(void) { ylc_runtime_service_global_deinit(); }

static const void *ylc_entry_get_factory(const char *factory_id) {
  if (!factory_id) {
    return NULL;
  }

  if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
    return &ylc_factory;
  }
  return NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = ylc_entry_init,
    .deinit = ylc_entry_deinit,
    .get_factory = ylc_entry_get_factory,
};
