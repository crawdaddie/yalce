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
  printf("vstfx node!! %s\n", path);
  // Initialize VST library if needed (you may want to do this once at startup)
  static int vst_initialized = 0;
  if (!vst_initialized) {
    vst_initialize();
    vst_initialized = 1;
  }

  // Get the AudioGraph
  AudioGraph *graph = _graph;
  if (!graph)
    return NULL;

  // Allocate node
  NodeRef node = allocate_node_in_graph(graph, sizeof(vst_state));
  if (!node)
    return NULL;

  // Get node index before we overwrite node
  int node_index = node->node_index;

  // Initialize node
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

  // Get samples per frame
  double spf = ctx_spf();

  // Connect input
  if (input) {
    node->connections[0].source_node_index = input->node_index;
    node->connections[0].input_index = 0;
  }

  // Initialize state
  vst_state *state =
      (vst_state *)(_graph->nodes_state_memory + node->state_offset);
  memset(state, 0, sizeof(vst_state));

  // Set up the input/output channel count
  state->num_input_channels = 2;  // Stereo
  state->num_output_channels = 2; // Stereo

  // Load the VST plugin
  VSTError err = vst_load_plugin(path, &state->plugin);
  if (err != VST_ERR_OK) {
    // Failed to load plugin
    printf("Failed to load VST plugin: %d\n", err);
    return node; // Return node anyway, it will output silence
  }

  // Get plugin info
  char plugin_name[128];
  vst_get_plugin_name(state->plugin, plugin_name, sizeof(plugin_name));
  printf("Loaded VST plugin: %s\n", plugin_name);

  // Setup processing
  uint32_t param_count = 0;
  vst_get_parameter_count(state->plugin, &param_count);
  printf("Plugin has %d parameters\n", param_count);

  // Setup channels
  VSTProcessSetup setup = {
      .sampleRate = (float)(1.0 / spf),
      .maxBlockSize = BUF_SIZE,
      .numInputChannels = 2, // Default to stereo in
      .numOutputChannels = 2 // Default to stereo out
  };

  vst_setup_processing(state->plugin, &setup);

  // Allocate input/output buffers
  state->num_input_channels = setup.numInputChannels;
  state->num_output_channels = setup.numOutputChannels;

  // Set node output to match plugin output channels
  node->output.layout = state->num_output_channels;

  node->connections[0].source_node_index = input->node_index;
  node->state_ptr = state;

  return node;
}

NodeRef vstfx_load_preset(const char *preset_path, NodeRef vst_node) {
  printf("loading preset %s\n", preset_path);
  VSTPluginHandle handle = ((vst_state *)((Node *)vst_node + 1))->plugin;
  vst_load_preset(handle, preset_path);
  return vst_node;
}

char *get_handle(NodeRef vst_node) {
  VSTPluginHandle handle = ((vst_state *)((Node *)vst_node + 1))->plugin;
  return handle;
}
