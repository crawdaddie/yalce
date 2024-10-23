#include "clap_node.h"
#include "clap/entry.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/params.h"
#include "clap/factory/plugin-factory.h"
#include "clap/process.h"
#include "common.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

const void *get_extension(const struct clap_host *host, const char *eid) {
  printf("Requesting Extension %s\n", eid);
  return NULL;
}

void request_restart(const struct clap_host *h) {}

void request_process(const struct clap_host *h) {}

void request_callback(const struct clap_host *h) {}

static clap_host_t host = {CLAP_VERSION_INIT,
                           NULL,
                           "YLC-ENGINE",
                           "Adam Juraszek",
                           "https://github.com/crawdaddie/yalce",
                           "0.0.0",
                           get_extension,
                           request_restart,
                           request_process,
                           request_callback};

typedef struct clap_plugin_state {
  const clap_plugin_t *plugin;
  clap_input_events_t in_events;
  clap_output_events_t out_events;
  int num_ins;
  float *big_input_buf;
  float *big_output_buf;
} clap_plugin_state;

clap_plugin_entry_t *load_clap(const char *path) {

  void *handle = dlopen(path, RTLD_LAZY);
  if (!handle) {
    fprintf(stderr, "Cannot open library: %s\n", dlerror());
    return NULL;
  }

  clap_plugin_entry_t *entry = dlsym(handle, "clap_entry");

  if (!entry) {
    fprintf(stderr, "Cannot find clap_entry symbol: %s\n", dlerror());
    dlclose(handle);
    return NULL;
  }

  printf("found clap plugin entry\n"
         "clap version: %d.%d.%d\n",
         entry->clap_version.major, entry->clap_version.minor,
         entry->clap_version.revision);
  return entry;
}

node_perform clap_perform(NodeRef node, int nframes, double spf) {
  clap_plugin_state *state = node->state;
  const clap_plugin_t *plugin = state->plugin;
  SignalRef input = node->ins;

  double *input_buf = node->ins[0].buf;
  double *out = node->out.buf;

  clap_process_t process;
  process.steady_time = -1;
  process.frames_count = nframes;
  process.transport = NULL; // we do need to fix this

  for (int i = 0; i < nframes; i++) {
    state->big_input_buf[0 * BUF_SIZE + i] = (float)input_buf[i];
    state->big_input_buf[1 * BUF_SIZE + i] = (float)input_buf[i];
  }
  clap_audio_buffer_t inputs[state->num_ins];
  inputs[0] = (clap_audio_buffer_t){
      .data32 =
          (float *[]){state->big_input_buf, state->big_input_buf + BUF_SIZE},
      .channel_count = 2,
  };

  clap_audio_buffer_t output = (clap_audio_buffer_t){
      .data32 =
          (float *[]){state->big_output_buf, state->big_output_buf + BUF_SIZE},
      .channel_count = 2,
  };

  process.in_events = &state->in_events;
  process.out_events = &state->out_events;

  process.audio_inputs = inputs;
  process.audio_inputs_count = state->num_ins;

  process.audio_outputs = &output;
  process.audio_outputs_count = 1;

  plugin->process(plugin, &process);

  for (int i = 0; i < nframes; i++) {
    *out = output.data32[0][i];
    *out += output.data32[1][i];
  }
}

uint32_t evts_size(const clap_input_events_t *evs) { return 0; }

static bool try_push(const struct clap_output_events *list,
                     const clap_event_header_t *event) {
  return true;
}
NodeRef clap_node(SignalRef input) {

  // clap_plugin_entry_t *entry = load_clap(
  //     "/Users/adam/Library/Audio/Plug-Ins/CLAP/blepfx/"
  //     "prisma-apple-universal.clap/Contents/MacOS/prisma-apple-universal");

  // clap_plugin_entry_t *entry =
  //     load_clap("/Users/adam/Library/Audio/Plug-Ins/CLAP/"
  //               "clap-c99-distortion.clap/Contents/MacOS/clap-c99-distortion");
  //
  // clap_plugin_entry_t *entry =
  //     load_clap("/Users/adam/projects/sound/clap-plugins/builds/ninja-headless/"
  //               "plugins/Debug/clap-plugins.clap/Contents/MacOS/clap-plugins");
  //
  // clap_plugin_entry_t *entry =
  //     load_clap("/Users/adam/projects/sound/elysiera/target/bundled/"
  //               "Elysiera.clap/Contents/MacOS/Elysiera");
  //
  clap_plugin_entry_t *entry =
      load_clap("/Library/Audio/Plug-Ins/CLAP/TAL-Reverb-4.clap/Contents/MacOS/"
                "TAL-Reverb-4");
  entry->init("Library/Audio/Plug-Ins/CLAP/TAL-Reverb-4.clap");

  clap_plugin_factory_t *fac = entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  uint32_t plugin_count = fac->get_plugin_count(fac);

  if (plugin_count <= 0) {
    printf("Plugin factory has no plugins");
  } else {
    printf("plugin count %d\n", plugin_count);
  }

  // const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 3);
  const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  printf("id: %s\n"
         "name: %s\n"
         "vendor: %s\n"
         "url: %s\n"
         "version: %s\n"
         "description: %s\n",
         desc->id, desc->name, desc->vendor, desc->url, desc->version,
         desc->description);

  // Now lets make an instance
  const clap_plugin_t *plugin = fac->create_plugin(fac, &host, desc->id);

  plugin->init(plugin);
  plugin->activate(plugin, 48000, 32, 4096);

  const clap_plugin_params_t *inst_params =
      plugin->get_extension(plugin, "clap.params");
  if (inst_params) {
    uint32_t pc = inst_params->count(plugin);

    printf("found %d params\n", pc);
    double param_vals[pc];

    for (uint32_t i = 0; i < pc; i++) {

      clap_param_info_t inf;
      inst_params->get_info(plugin, i, &inf);
      double d;
      inst_params->get_value(plugin, inf.id, &d);

      // printf("param %d %s %s %f %f %f\n", i, inf.name, inf.module,
      // inf.min_value, inf.max_value, inf.default_value);
      param_vals[i] = d;
      printf("%s %f\n", inf.name, param_vals[i]);
    }
  }
  const clap_plugin_audio_ports_t *inst_ports =
      plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS);
  int in_ports, out_ports;

  if (inst_ports) {
    in_ports = inst_ports->count(plugin, true);
    out_ports = inst_ports->count(plugin, false);

    for (int i = 0; i < in_ports; ++i) {
      clap_audio_port_info_t inf;
      inst_ports->get(plugin, i, true, &inf);
      printf("input %d (%s): [%d]\n", i, inf.name, inf.channel_count);
    }

    for (int i = 0; i < out_ports; ++i) {
      clap_audio_port_info_t inf;
      inst_ports->get(plugin, i, false, &inf);
      printf("output %d: [%d]\n", i, inf.channel_count);
    }
  }

  plugin->start_processing(plugin);
  clap_plugin_state *state = malloc(sizeof(clap_plugin_state));
  state->in_events.size = evts_size;
  state->out_events.try_push = try_push;

  state->plugin = plugin;
  state->num_ins = in_ports;
  state->big_input_buf = malloc(sizeof(float) * BUF_SIZE * in_ports * 2);
  state->big_output_buf = malloc(sizeof(float) * BUF_SIZE * out_ports * 2);

  Signal *ins = malloc(sizeof(Signal) * 1);
  ins[0].buf = input->buf;
  ins[0].size = input->size;
  ins[0].layout = input->layout;

  NodeRef node = node_new((void *)state, (node_perform *)clap_perform, 1, ins);
  return node;
}
