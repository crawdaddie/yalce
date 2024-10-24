#include "clap_node.h"
#include "clap_util.h"
#include "common.h"
#include "lib.h"
#include <clap/entry.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/params.h>
#include <clap/factory/plugin-factory.h>
#include <clap/process.h>
#include <dlfcn.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_bundle_executable(const char *bundle_path) {
  char plist_path[PATH_MAX];
  snprintf(plist_path, sizeof(plist_path), "%s/Contents/Info.plist",
           bundle_path);

  xmlDocPtr doc = xmlReadFile(plist_path, NULL, 0);
  if (doc == NULL) {
    return NULL;
  }

  xmlNodePtr root = xmlDocGetRootElement(doc);
  if (root == NULL) {
    xmlFreeDoc(doc);
    return NULL;
  }

  // Info.plist uses a dict structure where even nodes are keys and odd nodes
  // are values
  xmlNodePtr dict = root->children;
  while (dict != NULL) {
    if (dict->type == XML_ELEMENT_NODE &&
        strcmp((char *)dict->name, "dict") == 0) {
      break;
    }
    dict = dict->next;
  }

  if (dict == NULL) {
    xmlFreeDoc(doc);
    return NULL;
  }

  // Search for CFBundleExecutable key and get its value
  xmlNodePtr cur = dict->children;
  xmlNodePtr value_node = NULL;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
        strcmp((char *)cur->name, "key") == 0) {
      xmlChar *key = xmlNodeGetContent(cur);
      if (strcmp((char *)key, "CFBundleExecutable") == 0) {
        xmlFree(key);
        value_node = cur->next;
        while (value_node && value_node->type != XML_ELEMENT_NODE) {
          value_node = value_node->next;
        }
        break;
      }
      xmlFree(key);
    }
    cur = cur->next;
  }

  char *executable_name = NULL;
  if (value_node != NULL) {
    xmlChar *content = xmlNodeGetContent(value_node);
    if (content != NULL) {
      executable_name = strdup((char *)content);
      xmlFree(content);
    }
  }

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return executable_name;
}

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
#define MAX_EVENTS 1024

typedef struct event_queue {
  clap_event_header_t *events[MAX_EVENTS];
  uint32_t count;
} event_queue;

typedef struct clap_plugin_state {
  const clap_plugin_t *plugin;
  clap_input_events_t in_events;
  clap_output_events_t out_events;
  int num_ins;
  uint32_t param_count;
  float *input_buf;
  float *output_buf;
  event_queue input_queue;
  void *event_ctx;
} clap_plugin_state;

// Implementation of input events interface
uint32_t evts_size(const struct clap_input_events *list) {
  const clap_plugin_state *state = list->ctx;
  return state->input_queue.count;
}

const clap_event_header_t *evts_get(const struct clap_input_events *list,
                                    uint32_t index) {
  const clap_plugin_state *state = list->ctx;
  if (index < state->input_queue.count) {
    return state->input_queue.events[index];
  }
  return NULL;
}

// Helper function to create a parameter change event
clap_event_param_value_t *create_param_value_event(clap_id param_id,
                                                   double value) {
  clap_event_param_value_t *evt = calloc(1, sizeof(clap_event_param_value_t));
  evt->header.size = sizeof(clap_event_param_value_t);

  evt->header.time = 0;
  // evt->header.time = get_frame_offset(); // Immediate
  evt->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  evt->header.type = CLAP_EVENT_PARAM_VALUE;
  evt->header.flags = 0;
  evt->param_id = param_id;
  evt->value = value;
  return evt;
}

// Function to queue a parameter change
bool queue_parameter_change(clap_plugin_state *state, clap_id param_id,
                            double value) {
  if (state->input_queue.count >= MAX_EVENTS) {
    return false;
  }

  clap_event_param_value_t *evt = create_param_value_event(param_id, value);
  state->input_queue.events[state->input_queue.count++] = &evt->header;
  return true;
}

// Function to clear all events after processing
void clear_event_queue(clap_plugin_state *state) {
  for (uint32_t i = 0; i < state->input_queue.count; i++) {
    free(state->input_queue.events[i]);
  }
  state->input_queue.count = 0;
}

// Helper function to set a parameter with proper event handling
bool set_param_with_event(void *_state, const char *param_name, double value) {
  clap_plugin_state *state = _state;
  const clap_plugin_params_t *params =
      state->plugin->get_extension(state->plugin, CLAP_EXT_PARAMS);

  if (!params)
    return false;

  uint32_t param_count = params->count(state->plugin);
  clap_param_info_t param_info;

  // Find parameter by name
  for (uint32_t i = 0; i < param_count; i++) {
    if (!params->get_info(state->plugin, i, &param_info)) {
      continue;
    }

    if (strcmp(param_info.name, param_name) == 0) {
      // Clamp value to parameter range
      if (value < param_info.min_value)
        value = param_info.min_value;
      if (value > param_info.max_value)
        value = param_info.max_value;

      printf("setting %s %f [%f %f]\n", param_info.name, value,
             param_info.min_value, param_info.max_value);

      // Queue the parameter change event
      return queue_parameter_change(state, param_info.id, value);
    }
  }

  return false;
}

bool set_param_idx_with_event(void *_state, int idx, double value) {
  clap_plugin_state *state = _state;
  const clap_plugin_params_t *params =
      state->plugin->get_extension(state->plugin, CLAP_EXT_PARAMS);

  if (!params)
    return false;

  uint32_t param_count = params->count(state->plugin);
  clap_param_info_t param_info;
  if (!params->get_info(state->plugin, idx, &param_info)) {
    return false;
  }

  // Clamp value to parameter range
  if (value < param_info.min_value)
    value = param_info.min_value;
  if (value > param_info.max_value)
    value = param_info.max_value;

  printf("setting %s %f [%f %f]\n", param_info.name, value,
         param_info.min_value, param_info.max_value);

  // Queue the parameter change event
  return queue_parameter_change(state, param_info.id, value);
}

NodeRef set_clap_param(NodeRef node, int idx, double value) {

  clap_plugin_state *state = node->state;
  set_param_idx_with_event(state, idx, value);
  return node;
}

typedef struct audio_port_config {
  bool prefers_64bit;
  bool is_constant_mask; // If true, this config applies to all ports
  uint32_t channel_count;
  const char *port_name;
} audio_port_config;

audio_port_config get_port_config(const clap_plugin_t *plugin, bool is_input) {
  audio_port_config config = {.prefers_64bit = false,
                              .is_constant_mask = true,
                              .channel_count = 0,
                              .port_name = NULL};

  const clap_plugin_audio_ports_t *ports =
      plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS);

  if (!ports) {
    fprintf(stderr, "Plugin doesn't support audio ports extension\n");
    return config;
  }

  uint32_t port_count = ports->count(plugin, is_input);
  if (port_count == 0) {
    fprintf(stderr, "No %s ports found\n", is_input ? "input" : "output");
    return config;
  }

  // Get first port info to initialize our mask
  clap_audio_port_info_t port_info;
  if (!ports->get(plugin, 0, is_input, &port_info)) {
    fprintf(stderr, "Failed to get port info\n");
    return config;
  }

  config.prefers_64bit =
      (port_info.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) != 0;
  config.channel_count = port_info.channel_count;
  config.port_name = port_info.name;

  // Check if all ports have the same configuration
  for (uint32_t i = 1; i < port_count; i++) {
    if (!ports->get(plugin, i, is_input, &port_info)) {
      continue;
    }

    bool this_port_64bit =
        (port_info.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) != 0;
    if (this_port_64bit != config.prefers_64bit) {
      config.is_constant_mask = false;
      break;
    }
  }

  printf("%s port configuration:\n", is_input ? "Input" : "Output");
  printf("  Prefers 64-bit: %s\n", config.prefers_64bit ? "yes" : "no");
  printf("  Channel count: %d\n", config.channel_count);
  printf("  Port name: %s\n", config.port_name);
  printf("  Consistent across all ports: %s\n",
         config.is_constant_mask ? "yes" : "no");

  return config;
}

// Helper function to check both input and output ports
void check_plugin_data_preferences(const clap_plugin_t *plugin) {
  audio_port_config input_config = get_port_config(plugin, true);
  audio_port_config output_config = get_port_config(plugin, false);

  // Determine if we need to handle mixed formats
  bool mixed_formats =
      (input_config.prefers_64bit != output_config.prefers_64bit) ||
      !input_config.is_constant_mask || !output_config.is_constant_mask;

  if (mixed_formats) {
    printf(
        "\nWarning: Plugin uses mixed formats - will need format conversion\n");
  } else {
    printf("\nPlugin consistently prefers %s format\n",
           input_config.prefers_64bit ? "64-bit" : "32-bit");
  }
}

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
  SignalRef input_sig = node->ins;

  double *input = node->ins[0].buf;
  double *out = node->out.buf;

  clap_process_t process;
  process.steady_time = -1;
  process.frames_count = nframes;
  process.transport = NULL;

  // Copy input samples to the plugin's input buffer (assuming stereo)
  // Convert from interleaved to planar format
  for (int i = 0; i < nframes; i++) {
    // Left channel
    state->input_buf[i] = (float)input[i];
    // Right channel
    state->input_buf[i + nframes] = (float)input[i];
  }

  // Setup input buffers (planar format)
  clap_audio_buffer_t inputs[state->num_ins];
  inputs[0] = (clap_audio_buffer_t){
      .data32 =
          (float *[]){
              state->input_buf,          // Left channel
              state->input_buf + nframes // Right channel
          },
      .channel_count = 2,
  };
  if (state->num_ins > 1) {

    inputs[1] = (clap_audio_buffer_t){
        .data32 =
            (float *[]){
                state->input_buf + (2 * nframes), // Left channel
                state->input_buf + (3 * nframes)  // Right channel
            },
        .channel_count = 2,
    };
  }

  // Setup output buffers (planar format)
  clap_audio_buffer_t output = (clap_audio_buffer_t){
      .data32 =
          (float *[]){
              state->output_buf,          // Left channel
              state->output_buf + nframes // Right channel
          },
      .channel_count = 2,
  };

  process.in_events = &state->in_events;
  process.out_events = &state->out_events;
  process.audio_inputs = inputs;
  process.audio_inputs_count = state->num_ins;
  process.audio_outputs = &output;
  process.audio_outputs_count = 1;

  // Process audio through the plugin
  plugin->process(plugin, &process);

  // Copy output samples back (mixing stereo to mono)
  for (int i = 0; i < nframes; i++) {
    // Average left and right channels
    out[i] = (output.data32[0][i] + output.data32[1][i]);
    // printf("comp %f -> %f\n", input[i], out[i]);
  }
}

uint32_t get_param_num(NodeRef node) {
  return ((clap_plugin_state *)node->state)->param_count;
}

void export_param_specs(uint32_t pc, double *param_vals, double *min_vals,
                        double *max_vals, char **labels, NodeRef node) {
  clap_plugin_state *state = node->state;
  clap_plugin_t *plugin = state->plugin;
  const clap_plugin_params_t *inst_params =
      plugin->get_extension(plugin, "clap.params");

  for (uint32_t i = 0; i < pc; i++) {

    clap_param_info_t inf;
    inst_params->get_info(plugin, i, &inf);
    double d;
    inst_params->get_value(plugin, inf.id, &d);
    param_vals[i] = d;
    min_vals[i] = inf.min_value;
    max_vals[i] = inf.max_value;
    labels[i] = strdup(inf.name);
  }
}

static bool try_push(const struct clap_output_events *list,
                     const clap_event_header_t *event) {
  return true;
}
NodeRef clap_node(const char *plugin_path, SignalRef input) {
  char *executable_name = get_bundle_executable(plugin_path);
  if (!executable_name) {
    // Failed to get executable name from Info.plist
    return NULL;
  }

  // Construct the full path to the executable
  char full_path[PATH_MAX];
  snprintf(full_path, sizeof(full_path), "%s/Contents/MacOS/%s", plugin_path,
           executable_name);

  // Load the plugin
  clap_plugin_entry_t *entry = load_clap(full_path);

  entry->init(plugin_path);

  const clap_plugin_factory_t *fac = entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
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

  // Or get specific port configs:
  audio_port_config input_cfg = get_port_config(plugin, true);

  plugin->init(plugin);
  plugin->activate(plugin, 48000, 32, 4096);
  check_plugin_data_preferences(plugin);

  const clap_plugin_params_t *inst_params =
      plugin->get_extension(plugin, "clap.params");

  uint32_t pc = 0;
  if (inst_params) {
    pc = inst_params->count(plugin);

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
      printf("%d: %s %f\n", i, inf.name, param_vals[i]);
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
  // Initialize event handling
  state->input_queue.count = 0;
  state->event_ctx = state; // Point ctx to the state itself
  state->param_count = pc;

  // Set up input events interface
  state->in_events.ctx = state;
  state->in_events.size = evts_size;
  state->in_events.get = evts_get;

  // Set up output events interface
  state->out_events.ctx = state;
  state->out_events.try_push = try_push;

  state->plugin = plugin;
  state->num_ins = in_ports;
  state->input_buf = calloc(BUF_SIZE * in_ports * 2, sizeof(float));
  state->output_buf = calloc(BUF_SIZE * out_ports * 2, sizeof(float));

  NodeRef node =
      node_new((void *)state, (node_perform *)clap_perform, 1, input);
  return node;
}
