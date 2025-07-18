#include "../audio_graph.h"
#include "../common.h" // For BUF_SIZE and other constants
#include "../node.h"
#include <VSTPlugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern AudioGraph *_graph;   // Assume this is defined elsewhere
extern double ctx_spf(void); // Function to get samples per frame

// State structure for VST nodes
#define MAX_VST_CHANNELS 2 // For stereo

typedef struct {
  VSTPluginHandle plugin;
  int num_input_channels;
  int num_output_channels;
  int initialized;
  Node *input;
  // Fixed-size buffer storage
  float input_buffers[2 * MAX_VST_CHANNELS * BUF_SIZE];
} vst_state;

void *vstfx_perform(Node *node, vst_state *state, Node **inputs, int nframes,
                    double spf) {

  if (!state || !state->plugin || !node)
    return NULL;

  float *channel_ptrs[MAX_VST_CHANNELS * 2];
  float **input_channel_ptrs = channel_ptrs;
  float **output_channel_ptrs = channel_ptrs + MAX_VST_CHANNELS;

  // if (state->input) {
  //   inputs = &state->input;
  //   printf("perform vst state input = %p\n", *inputs);
  // }

  float *buffer_memory = state->input_buffers;

  for (int c = 0; c < state->num_input_channels; c++) {
    input_channel_ptrs[c] = buffer_memory + (c * BUF_SIZE);

    if (inputs && inputs[0]) {
      int in_layout = inputs[0]->output.layout;
      // Convert from interleaved double to non-interleaved float
      for (int i = 0; i < nframes; i++) {

        input_channel_ptrs[c][i] =
            (float)inputs[0]->output.buf[i * in_layout + c];
      }
    } else {
      // No input, fill with silence
      memset(input_channel_ptrs[c], 0, nframes * sizeof(float));
    }
  }

  for (int c = 0; c < state->num_output_channels; c++) {
    output_channel_ptrs[c] =
        buffer_memory + ((MAX_VST_CHANNELS + c) * BUF_SIZE);
    memset(output_channel_ptrs[c], 0, nframes * sizeof(float));
  }

  VSTError err =
      vst_process_audio(state->plugin, (const float **)input_channel_ptrs,
                        output_channel_ptrs, nframes);

  if (err != VST_ERR_OK) {
    memset(node->output.buf, 0, nframes * node->output.layout * sizeof(double));
    return NULL;
  }

  for (int c = 0; c < node->output.layout && c < state->num_output_channels;
       c++) {
    for (int i = 0; i < nframes; i++) {
      node->output.buf[i * node->output.layout + c] =
          (double)output_channel_ptrs[c][i];
    }
  }

  return NULL;
}

// Cleanup function for VST nodes
void vstfx_cleanup(vst_state *state) {
  if (!state)
    return;

  // Unload the plugin
  if (state->plugin) {
    vst_unload_plugin(state->plugin);
  }
}

// Create a VST plugin node
NodeRef vstfx_node(const char *path, NodeRef input) {
  static int vst_initialized = 0;
  if (!vst_initialized) {
    vst_initialize();
    vst_initialized = 1;
  }
  AudioGraph *graph = _graph;

  NodeRef node = allocate_node_in_graph(graph, sizeof(vst_state));

  // Get node index before we overwrite node
  int node_index = node->node_index;

  *node = (Node){
      .perform = (perform_func_t)vstfx_perform,
      .node_index = node_index,
      .num_inputs = 1,
      .state_size = sizeof(vst_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(vst_state)),
      .output = {.layout = 2, // Default to stereo output
                 .size = BUF_SIZE,
                 .buf = allocate_buffer_from_pool(graph, 2 * BUF_SIZE)},
      .meta = "vst_plugin",
  };

  double spf = ctx_spf();

  vst_state *state;
  if (_graph) {
    state = (vst_state *)(_graph->nodes_state_memory + node->state_offset);
  } else {
    state = (vst_state *)(node + 1);
  }
  memset(state, 0, sizeof(vst_state));

  state->num_input_channels = 2;
  state->num_output_channels = 2;

  VSTError err = vst_load_plugin(path, &state->plugin);
  if (err != VST_ERR_OK) {
    printf("Failed to load VST plugin: %d\n", err);
    return node;
  }
  printf("plugin handle is at %p\n", state->plugin);

  char plugin_name[128];
  vst_get_plugin_name(state->plugin, plugin_name, sizeof(plugin_name));
  printf("Loaded VST plugin: %s\n", plugin_name);
  printf("Plugin input: %p\n", input);

  uint32_t param_count = 0;
  vst_get_parameter_count(state->plugin, &param_count);
  printf("Plugin has %d parameters\n", param_count);

  VSTProcessSetup setup = {
      .sampleRate = (float)(1.0 / spf),
      .maxBlockSize = BUF_SIZE,
      .numInputChannels = 2, // Default to stereo in
      .numOutputChannels = 2 // Default to stereo out
  };

  vst_setup_processing(state->plugin, &setup);

  state->num_input_channels = setup.numInputChannels;
  state->num_output_channels = setup.numOutputChannels;

  node->output.layout = state->num_output_channels;

  node->state_ptr = state;

  plug_input_in_graph(0, node, input);
  return node;
}

NodeRef vstfx_load_preset(const char *preset_path, NodeRef vst_node) {
  printf("loading preset %s\n", preset_path);
  VSTPluginHandle handle = ((vst_state *)((Node *)vst_node + 1))->plugin;
  vst_load_preset(handle, preset_path);
  return vst_node;
}

char *vst_get_handle(NodeRef vst_node) {
  vst_state *state = vst_node->state_ptr;
  VSTPluginHandle handle = state->plugin;
  printf("got handle to return %p\n", handle);
  return handle;
}
