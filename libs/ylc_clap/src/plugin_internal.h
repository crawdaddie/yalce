#ifndef YLC_CLAP_PLUGIN_INTERNAL_H
#define YLC_CLAP_PLUGIN_INTERNAL_H

#include "clap/ext/posix-fd-support.h"
#include "clap/ext/gui.h"
#include "clap/ext/state.h"
#include "clap/plugin.h"
#include "clap/process.h"

typedef struct MIDI_eventlist MIDI_eventlist;
typedef struct accelerator_register_t accelerator_register_t;
typedef struct ProjectStateContext ProjectStateContext;
typedef struct PCM_source PCM_source;

#include "reaper/reaper_plugin.h"

#include "audio_graph.h"
#include "runtime_service.h"
#include "scheduler.h"
#include "script_runtime.h"
#include "soundfile.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef struct _XGC *GC;

#define YLC_PLUGIN_ID "dev.ylc.script-shim"
#define YLC_PARAM_COUNT 32
#define YLC_PARAM_BASE_ID 1000
#define YLC_PARAM_FIRST_INDEX 0
#define YLC_REAPER_EXTENSION_ID "cockos.reaper_extension"
#define YLC_SCRIPT_PATH_SIZE 1024
#define YLC_GUI_WIDTH 560
#define YLC_GUI_HEIGHT 720
#define YLC_PATH_X 16
#define YLC_PATH_Y 42
#define YLC_PATH_W 528
#define YLC_PATH_H 28
#define YLC_OPEN_BUTTON_X 16
#define YLC_OPEN_BUTTON_Y 84
#define YLC_OPEN_BUTTON_W 148
#define YLC_OPEN_BUTTON_H 30
#define YLC_LOG_BUTTON_X 180
#define YLC_LOG_BUTTON_Y 84
#define YLC_LOG_BUTTON_W 148
#define YLC_LOG_BUTTON_H 30
#define YLC_STATUS_X 16
#define YLC_STATUS_Y 142
#define YLC_ARRAY_EDITOR_X 16
#define YLC_ARRAY_EDITOR_Y 164
#define YLC_ARRAY_EDITOR_W 528
#define YLC_ARRAY_EDITOR_H 156
#define YLC_ARRAY_EDITOR_GAP 12
#define YLC_DEBUG_LINES 6
#define YLC_DEBUG_LINE_SIZE 160
#define YLC_INPUT_EVENT_LOG_CAPACITY 128
#define YLC_PERSIST_ARRAY_MAX_SLOTS 1024u
#define YLC_PERSIST_ARRAY_MAX_COUNT 1048576u
#define YLC_UI_MAX_SLOTS 64u
#define YLC_STATE_MAGIC 0x594c4350u
#define YLC_STATE_VERSION 1u

typedef enum ylc_input_event_log_type {
  YLC_INPUT_EVENT_NOTE_ON,
  YLC_INPUT_EVENT_NOTE_OFF,
  YLC_INPUT_EVENT_NOTE_CHOKE,
  YLC_INPUT_EVENT_NOTE_EXPRESSION,
  YLC_INPUT_EVENT_PARAM_VALUE,
  YLC_INPUT_EVENT_PARAM_MOD,
  YLC_INPUT_EVENT_PARAM_GESTURE_BEGIN,
  YLC_INPUT_EVENT_PARAM_GESTURE_END,
  YLC_INPUT_EVENT_TRANSPORT,
  YLC_INPUT_EVENT_MIDI,
  YLC_INPUT_EVENT_MIDI_SYSEX,
  YLC_INPUT_EVENT_MIDI2,
  YLC_INPUT_EVENT_UNKNOWN,
} ylc_input_event_log_type_t;

typedef struct ylc_input_event_log_record {
  ylc_input_event_log_type_t type;
  uint32_t sample_offset;
  uint16_t clap_type;
  clap_id param_id;
  int32_t note_id;
  int16_t port_index;
  int16_t channel;
  int16_t key;
  double value;
  double aux_value;
  uint8_t midi[3];
  uint32_t size;
} ylc_input_event_log_record_t;

typedef struct ylc_event_handlers {
  void *user_data;
  void (*on_note_on)(void *user_data, const clap_event_note_t *event);
  void (*on_note_off)(void *user_data, const clap_event_note_t *event);
  void (*on_note_choke)(void *user_data, const clap_event_note_t *event);
  void (*on_note_expression)(void *user_data,
                             const clap_event_note_expression_t *event);
  void (*on_param_value)(void *user_data,
                         const clap_event_param_value_t *event);
  void (*on_param_mod)(void *user_data, const clap_event_param_mod_t *event);
  void (*on_param_gesture)(void *user_data,
                           const clap_event_param_gesture_t *event);
  void (*on_transport)(void *user_data, const clap_event_transport_t *event);
  void (*on_midi)(void *user_data, const clap_event_midi_t *event);
  void (*on_midi_sysex)(void *user_data, const clap_event_midi_sysex_t *event);
  void (*on_midi2)(void *user_data, const clap_event_midi2_t *event);
  void (*on_unknown)(void *user_data, const clap_event_header_t *event);
} ylc_event_handlers_t;

typedef struct ylc_persistent_array_slot {
  uint64_t key;
  uint32_t count;
  double *values;
} ylc_persistent_array_slot_t;

typedef enum ylc_ui_kind {
  YLC_UI_NONE = 0,
  YLC_UI_ENV = 1,
  YLC_UI_ADSR = 2,
  YLC_UI_SOUNDFILE = 3,
} ylc_ui_kind_t;

typedef struct ylc_ui_slot {
  ylc_ui_kind_t kind;
  uint32_t array_count;
  double *array_values;
  ylc_soundfile_t *soundfile;
  int x, y, w, h;
} ylc_ui_slot_t;

typedef void *(*ylc_clap_get_reaper_context_fn)(const clap_host_t *host,
                                                int sel);

typedef struct ylc_plugin {
  clap_plugin_t plugin;
  const clap_host_t *host;
  const clap_host_state_t *host_state;
  const clap_host_posix_fd_support_t *host_posix_fd;
  const reaper_plugin_info_t *reaper;
  ylc_clap_get_reaper_context_fn clap_get_reaper_context;
  void *reaper_parent_track;
  void *reaper_project;
  ylc_runtime_service_t *runtime_service;
  uint32_t instance_id;
  double sample_rate;
  double tempo_bpm;
  uint32_t max_frames_count;
  bool processing;
  bool transport_playing;
  bool clap_initialized;
  bool destroying;
  double param_values[YLC_PARAM_COUNT];
  ylc_persistent_array_slot_t *persistent_arrays;
  uint32_t persistent_array_count;
  uint32_t persistent_array_capacity;
  ylc_ui_slot_t ui_slots[YLC_UI_MAX_SLOTS];
  uint32_t ui_count;
  ylc_soundfile_slot_t *soundfiles;
  uint32_t soundfile_count;
  uint32_t soundfile_capacity;
  ylc_soundfile_inherit_t *sf_inherit;
  uint32_t sf_inherit_count;
  uint32_t sf_inherit_index;
  bool sf_inherit_from_state;
  char script_path[YLC_SCRIPT_PATH_SIZE];
  char compiled_script_path[YLC_SCRIPT_PATH_SIZE];
  char watched_dir[YLC_SCRIPT_PATH_SIZE];
  char watched_name[YLC_SCRIPT_PATH_SIZE];
  int inotify_fd;
  int inotify_wd;
  bool inotify_registered;
  bool script_program_ready;
  atomic_bool script_reload_pending;
  Display *display;
  Window parent_window;
  Window gui_window;
  GC gc;
  bool gui_created;
  bool gui_visible;
  bool path_focused;
  int32_t gui_selected_array;
  int32_t gui_selected_point;
  bool gui_dragging;
  int32_t sf_dragging_edge;
  int sf_drag_start_x;
  uint64_t sf_drag_start_rs;
  uint64_t sf_drag_start_re;
  unsigned long dnd_aware;
  unsigned long dnd_enter;
  unsigned long dnd_position;
  unsigned long dnd_status;
  unsigned long dnd_drop;
  unsigned long dnd_leave;
  unsigned long dnd_finished;
  unsigned long dnd_selection;
  unsigned long dnd_action_copy;
  unsigned long dnd_uri_list;
  unsigned long dnd_property;
  Window dnd_source;
  int dnd_mouse_x;
  int dnd_mouse_y;
  int debug_pipe_read_fd;
  int debug_pipe_write_fd;
  bool debug_pipe_registered;
  FILE *debug_stream;
  FILE *debug_log_file;
  char debug_log_path[YLC_SCRIPT_PATH_SIZE];
  char debug_partial[YLC_DEBUG_LINE_SIZE];
  size_t debug_partial_len;
  char debug_lines[YLC_DEBUG_LINES][YLC_DEBUG_LINE_SIZE];
  uint32_t debug_start;
  uint32_t debug_count;
  uint32_t debug_seq;
  ylc_event_handlers_t event_handlers;
  ylc_input_event_log_record_t input_event_log[YLC_INPUT_EVENT_LOG_CAPACITY];
  atomic_uint input_event_log_write_seq;
  atomic_uint input_event_log_read_seq;
  atomic_uint input_event_log_dropped;
  atomic_uintptr_t midi_in_handler;
  atomic_uintptr_t param_in_handler;
  atomic_uintptr_t param_mod_in_handler;
  atomic_uintptr_t param_gesture_in_handler;
  atomic_uint active_process_count;
  atomic_bool compile_in_progress;
  ylc_program_t fallback_program;
  ylc_dummy_audio_graph_t jit_dummy_graph;
  ylc_clap_scheduler_t scheduler;
  _Atomic(ylc_program_t *) active_program;
} ylc_plugin_t;

ylc_plugin_t *ylc_from_plugin(const clap_plugin_t *plugin);

void ylc_mark_state_dirty(ylc_plugin_t *self);
void ylc_setup_script_watcher(ylc_plugin_t *self);
void ylc_spawn_editor(ylc_plugin_t *self);
void ylc_spawn_log_follower(ylc_plugin_t *self);

void ylc_gui_draw(ylc_plugin_t *self);
void ylc_gui_poll_events(ylc_plugin_t *self);
void ylc_gui_close(ylc_plugin_t *self);
const clap_plugin_gui_t *ylc_gui_extension(void);

#endif
