#include "clap/entry.h"
#include "clap/factory/plugin-factory.h"
#include "clap/plugin.h"
#include "clap/process.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct process_thread_args {
  const clap_plugin_t *plugin;
  clap_process_t process;
  clap_audio_buffer_t output;
  double out0[256];
  double out1[256];
  double *outs[2];
  atomic_bool stop;
  atomic_uint blocks;
} process_thread_args_t;

typedef struct smoke_input_events {
  uint32_t event_count;
  bool delivered;
  const clap_event_header_t *headers[4];
  clap_event_midi_t midi;
  clap_event_note_t note_on;
  clap_event_note_t note_off;
  clap_event_param_value_t param_value;
} smoke_input_events_t;

static const clap_event_header_t *empty_events_get(const clap_input_events_t *l,
                                                   uint32_t index) {
  (void)l;
  (void)index;
  return NULL;
}

static uint32_t empty_events_size(const clap_input_events_t *l) {
  (void)l;
  return 0;
}

static const clap_event_header_t *smoke_events_get(const clap_input_events_t *l,
                                                   uint32_t index) {
  smoke_input_events_t *events =
      l ? (smoke_input_events_t *)l->ctx : NULL;
  if (!events || events->delivered || index >= events->event_count) {
    return NULL;
  }

  if (index + 1 >= events->event_count) {
    events->delivered = true;
  }
  return events->headers[index];
}

static uint32_t smoke_events_size(const clap_input_events_t *l) {
  smoke_input_events_t *events =
      l ? (smoke_input_events_t *)l->ctx : NULL;
  return events && !events->delivered ? events->event_count : 0u;
}

static const void *host_get_extension(const clap_host_t *host,
                                      const char *extension_id) {
  (void)host;
  (void)extension_id;
  return NULL;
}

static void host_request_restart(const clap_host_t *host) { (void)host; }
static void host_request_process(const clap_host_t *host) { (void)host; }
static void host_request_callback(const clap_host_t *host) { (void)host; }

static void *process_thread_main(void *arg_raw) {
  process_thread_args_t *arg = (process_thread_args_t *)arg_raw;
  while (!atomic_load_explicit(&arg->stop, memory_order_acquire)) {
    memset(arg->out0, 0, sizeof(arg->out0));
    memset(arg->out1, 0, sizeof(arg->out1));
    (void)arg->plugin->process(arg->plugin, &arg->process);
    arg->process.steady_time += arg->process.frames_count;
    atomic_fetch_add_explicit(&arg->blocks, 1, memory_order_relaxed);
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <script.ylc> [plugin.clap]\n", argv[0]);
    return 2;
  }

  const char *script_path = argv[1];
  const char *plugin_path =
      argc > 2 ? argv[2] : "libs/ylc_clap/build/ylc_script.clap";
  if (access(script_path, R_OK) != 0) {
    perror("script path is not readable");
    return 1;
  }

  setenv("YLC_SCRIPT_PATH", script_path, 1);

  void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    return 1;
  }

  const clap_plugin_entry_t *entry =
      (const clap_plugin_entry_t *)dlsym(handle, "clap_entry");
  if (!entry) {
    fprintf(stderr, "missing clap_entry: %s\n", dlerror());
    return 1;
  }

  if (!entry->init(plugin_path)) {
    fprintf(stderr, "entry init failed\n");
    return 1;
  }

  const clap_plugin_factory_t *factory =
      (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!factory) {
    fprintf(stderr, "missing plugin factory\n");
    return 1;
  }

  clap_host_t host = {
      .clap_version = CLAP_VERSION_INIT,
      .host_data = NULL,
      .name = "ylc-host-smoke",
      .vendor = "ylc",
      .url = "",
      .version = "0",
      .get_extension = host_get_extension,
      .request_restart = host_request_restart,
      .request_process = host_request_process,
      .request_callback = host_request_callback,
  };

  const clap_plugin_t *plugin =
      factory->create_plugin(factory, &host, "dev.ylc.script-shim");
  if (!plugin) {
    fprintf(stderr, "create_plugin failed\n");
    return 1;
  }
  if (!plugin->init(plugin)) {
    fprintf(stderr, "plugin init failed\n");
    return 1;
  }
  if (!plugin->activate(plugin, 48000.0, 1, 256)) {
    fprintf(stderr, "activate failed\n");
    return 1;
  }
  if (!plugin->start_processing(plugin)) {
    fprintf(stderr, "start_processing failed\n");
    return 1;
  }

  double out0[256] = {0};
  double out1[256] = {0};
  double *outs[2] = {out0, out1};
  clap_audio_buffer_t output = {
      .data32 = NULL,
      .data64 = outs,
      .channel_count = 2,
      .latency = 0,
      .constant_mask = 0,
  };
  const bool send_midi = getenv("YLC_SMOKE_MIDI") != NULL;
  const bool send_note = getenv("YLC_SMOKE_NOTE") != NULL;
  const bool send_param = getenv("YLC_SMOKE_PARAM") != NULL;
  const bool transport_stopped =
      getenv("YLC_SMOKE_TRANSPORT_STOPPED") != NULL;
  smoke_input_events_t smoke_events = {
      .midi =
          {
              .header =
                  {
                      .size = sizeof(clap_event_midi_t),
                      .time = 8,
                      .space_id = CLAP_CORE_EVENT_SPACE_ID,
                      .type = CLAP_EVENT_MIDI,
                      .flags = 0,
                  },
              .port_index = 0,
              .data = {0x90, 60, 100},
          },
      .note_on =
          {
              .header =
                  {
                      .size = sizeof(clap_event_note_t),
                      .time = 8,
                      .space_id = CLAP_CORE_EVENT_SPACE_ID,
                      .type = CLAP_EVENT_NOTE_ON,
                      .flags = 0,
                  },
              .note_id = -1,
              .port_index = 0,
              .channel = 0,
              .key = 60,
              .velocity = 0.75,
          },
      .note_off =
          {
              .header =
                  {
                      .size = sizeof(clap_event_note_t),
                      .time = 32,
                      .space_id = CLAP_CORE_EVENT_SPACE_ID,
                      .type = CLAP_EVENT_NOTE_OFF,
                      .flags = 0,
                  },
              .note_id = -1,
              .port_index = 0,
              .channel = 0,
              .key = 60,
              .velocity = 0.0,
          },
      .param_value =
          {
              .header =
                  {
                      .size = sizeof(clap_event_param_value_t),
                      .time = 12,
                      .space_id = CLAP_CORE_EVENT_SPACE_ID,
                      .type = CLAP_EVENT_PARAM_VALUE,
                      .flags = 0,
                  },
              .param_id = 1000,
              .cookie = NULL,
              .note_id = -1,
              .port_index = -1,
              .channel = -1,
              .key = -1,
              .value = 0.42,
          },
  };
  if (send_midi) {
    smoke_events.headers[smoke_events.event_count++] =
        &smoke_events.midi.header;
  }
  if (send_note) {
    smoke_events.headers[smoke_events.event_count++] =
        &smoke_events.note_on.header;
    smoke_events.headers[smoke_events.event_count++] =
        &smoke_events.note_off.header;
  }
  if (send_param) {
    smoke_events.headers[smoke_events.event_count++] =
        &smoke_events.param_value.header;
  }
  clap_input_events_t in_events = smoke_events.event_count > 0
                                      ? (clap_input_events_t){
                                            .ctx = &smoke_events,
                                            .size = smoke_events_size,
                                            .get = smoke_events_get,
                                        }
                                      : (clap_input_events_t){
                                            .ctx = NULL,
                                            .size = empty_events_size,
                                            .get = empty_events_get,
                                        };
  clap_event_transport_t transport = {
      .header = {.size = sizeof(transport),
                 .time = 0,
                 .space_id = CLAP_CORE_EVENT_SPACE_ID,
                 .type = CLAP_EVENT_TRANSPORT,
                 .flags = 0},
      .flags = CLAP_TRANSPORT_HAS_TEMPO |
               (transport_stopped ? 0u : CLAP_TRANSPORT_IS_PLAYING),
      .tempo = 120.0,
      .tsig_num = 4,
      .tsig_denom = 4,
  };
  clap_process_t process = {
      .steady_time = 0,
      .frames_count = 256,
      .transport = &transport,
      .audio_inputs = NULL,
      .audio_outputs = &output,
      .audio_inputs_count = 0,
      .audio_outputs_count = 1,
      .in_events = &in_events,
      .out_events = NULL,
  };

  clap_process_status status = plugin->process(plugin, &process);
  printf("status=%d first=%f mid=%f last=%f\n", status, out0[0], out0[64],
         out0[255]);

  plugin->on_main_thread(plugin);
  memset(out0, 0, sizeof(out0));
  memset(out1, 0, sizeof(out1));
  process.steady_time += process.frames_count;
  status = plugin->process(plugin, &process);
  printf("callback_status=%d first=%f mid=%f last=%f\n", status, out0[0],
         out0[64], out0[255]);

  process_thread_args_t thread_args = {
      .plugin = plugin,
      .process = process,
  };
  thread_args.outs[0] = thread_args.out0;
  thread_args.outs[1] = thread_args.out1;
  thread_args.output = output;
  thread_args.output.data64 = thread_args.outs;
  thread_args.process.audio_outputs = &thread_args.output;
  atomic_init(&thread_args.stop, false);
  atomic_init(&thread_args.blocks, 0);

  pthread_t process_thread;
  if (pthread_create(&process_thread, NULL, process_thread_main,
                     &thread_args) != 0) {
    perror("pthread_create");
    return 1;
  }

  for (int i = 0; i < 8; ++i) {
    usleep(10000);
    plugin->on_main_thread(plugin);
  }

  atomic_store_explicit(&thread_args.stop, true, memory_order_release);
  pthread_join(process_thread, NULL);
  printf("concurrent_blocks=%u\n",
         atomic_load_explicit(&thread_args.blocks, memory_order_relaxed));

  plugin->stop_processing(plugin);
  plugin->deactivate(plugin);
  plugin->destroy(plugin);
  entry->deinit();
  dlclose(handle);
  return 0;
}
