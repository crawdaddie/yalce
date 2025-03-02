#include "audio_loop.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PI 3.14159265358979323846
#define MAX_NODES 16
#define MAX_INPUTS 4
#define BUF_SIZE 1024 // Assuming this is defined in your headers

// Forward declarations
typedef void *(*perform_func_t)(void *ptr, int out_layout, double *out,
                                int nframes, double spf, double **inputs);

// Node types
typedef enum {
  NODE_SIN,
  NODE_GAIN,
  NODE_MIX,
  // Add more node types as needed
} NodeType;

// Graph structure in memory blob format
typedef struct {
  int node_count;                           // Number of nodes in the graph
  int execution_order[MAX_NODES];           // Order to execute nodes
  int layouts[MAX_NODES];                   // Output layout for each node
  int input_counts[MAX_NODES];              // Number of inputs for each node
  int input_indices[MAX_NODES][MAX_INPUTS]; // Indices of input nodes
  size_t node_offsets[MAX_NODES];   // Offset to each node's data in the blob
  size_t output_offsets[MAX_NODES]; // Offset to each node's output buffer
  NodeType node_types[MAX_NODES];   // Type of each node
} GraphHeader;

// Node structure for graph building (not stored in blob)
typedef struct {
  NodeType type;
  int layout;
  int input_count;
  int inputs[MAX_INPUTS];
  void *state;
  size_t state_size;
  perform_func_t perform_func;
} NodeDef;

// Sin oscillator state
typedef struct {
  double phase;
  double freq;
} SinState;

// Gain node state
typedef struct {
  double gain;
} GainState;

// Mixer node state
typedef struct {
  double gains[MAX_INPUTS];
} MixState;

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    sin_table[i] = sin(phase);
    phase += phsinc;
  }
}

// Sin oscillator perform function
void *sin_perform(void *ptr, int out_layout, double *out, int nframes,
                  double spf, double **inputs) {
  SinState *state = (SinState *)ptr;
  ptr += sizeof(SinState);

  double d_index;
  int index = 0;
  double frac, a, b, sample;

  while (nframes--) {
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % SIN_TABSIZE];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->freq * spf + state->phase, 1.0);
  }

  return ptr;
}

// Gain node perform function
void *gain_perform(void *ptr, int out_layout, double *out, int nframes,
                   double spf, double **inputs) {
  GainState *state = (GainState *)ptr;
  ptr += sizeof(GainState);

  double *input = inputs[0];

  if (!input) {
    // No input, output silence
    while (nframes--) {
      for (int i = 0; i < out_layout; i++) {
        *out++ = 0.0;
      }
    }
    return ptr;
  }

  while (nframes--) {
    for (int i = 0; i < out_layout; i++) {
      *out++ = *input++ * state->gain;
    }
  }

  return ptr;
}

// Mixer node perform function
void *mix_perform(void *ptr, int out_layout, double *out, int nframes,
                  double spf, double **inputs) {
  MixState *state = (MixState *)ptr;
  ptr += sizeof(MixState);

  int input_count = 0;
  while (inputs[input_count] && input_count < MAX_INPUTS) {
    input_count++;
  }

  // Zero output buffer
  for (int i = 0; i < nframes * out_layout; i++) {
    out[i] = 0.0;
  }

  // Mix inputs
  for (int in_idx = 0; in_idx < input_count; in_idx++) {
    double *input = inputs[in_idx];
    if (!input)
      continue;

    double gain = state->gains[in_idx];
    double *out_ptr = out;

    for (int n = 0; n < nframes; n++) {
      for (int i = 0; i < out_layout; i++) {
        *out_ptr++ += *input++ * gain;
      }
    }
  }

  return ptr;
}

// Get perform function by node type
perform_func_t get_perform_func(NodeType type) {
  switch (type) {
  case NODE_SIN:
    return sin_perform;
  case NODE_GAIN:
    return gain_perform;
  case NODE_MIX:
    return mix_perform;
  default:
    return NULL;
  }
}

// Get state size by node type
size_t get_state_size(NodeType type) {
  switch (type) {
  case NODE_SIN:
    return sizeof(SinState);
  case NODE_GAIN:
    return sizeof(GainState);
  case NODE_MIX:
    return sizeof(MixState);
  default:
    return 0;
  }
}

// Create Sin oscillator node
int create_sin_node(NodeDef *node_defs, int *node_count, int layout,
                    double freq) {
  int idx = (*node_count)++;
  NodeDef *node = &node_defs[idx];

  node->type = NODE_SIN;
  node->layout = layout;
  node->input_count = 0;
  node->perform_func = sin_perform;

  SinState *state = (SinState *)malloc(sizeof(SinState));
  state->phase = 0.0;
  state->freq = freq;

  node->state = state;
  node->state_size = sizeof(SinState);

  return idx;
}

// Create Gain node
int create_gain_node(NodeDef *node_defs, int *node_count, int layout,
                     int input_idx, double gain) {
  int idx = (*node_count)++;
  NodeDef *node = &node_defs[idx];

  node->type = NODE_GAIN;
  node->layout = layout;
  node->input_count = 1;
  node->inputs[0] = input_idx;
  node->perform_func = gain_perform;

  GainState *state = (GainState *)malloc(sizeof(GainState));
  state->gain = gain;

  node->state = state;
  node->state_size = sizeof(GainState);

  return idx;
}

// Create Mix node
int create_mix_node(NodeDef *node_defs, int *node_count, int layout,
                    int input_count, int *input_indices, double *gains) {
  int idx = (*node_count)++;
  NodeDef *node = &node_defs[idx];

  node->type = NODE_MIX;
  node->layout = layout;
  node->input_count = input_count;

  for (int i = 0; i < input_count; i++) {
    node->inputs[i] = input_indices[i];
  }

  node->perform_func = mix_perform;

  MixState *state = (MixState *)malloc(sizeof(MixState));
  for (int i = 0; i < input_count; i++) {
    state->gains[i] = gains[i];
  }

  node->state = state;
  node->state_size = sizeof(MixState);

  return idx;
}

// Determine the execution order of nodes
void determine_execution_order(NodeDef *node_defs, int node_count,
                               int *execution_order) {
  // Simple topological sort
  int visited[MAX_NODES] = {0};
  int order_idx = 0;

  // Helper function for depth-first traversal
  void visit(int node_idx) {
    if (visited[node_idx])
      return;
    visited[node_idx] = 1;

    // Visit all inputs first
    for (int i = 0; i < node_defs[node_idx].input_count; i++) {
      visit(node_defs[node_idx].inputs[i]);
    }

    // Add this node to the execution order
    execution_order[order_idx++] = node_idx;
  }

  // Visit all nodes
  for (int i = 0; i < node_count; i++) {
    visit(i);
  }
}

// Calculate total blob size needed
size_t calculate_blob_size(NodeDef *node_defs, int node_count) {
  size_t size = sizeof(GraphHeader);

  // Add size for each node's state and function pointer
  for (int i = 0; i < node_count; i++) {
    size += sizeof(perform_func_t) + node_defs[i].state_size;
  }

  // Add size for output buffers
  for (int i = 0; i < node_count; i++) {
    size += node_defs[i].layout * BUF_SIZE * sizeof(double);
  }

  return size;
}

// Pack graph into a blob
void *pack_graph(NodeDef *node_defs, int node_count, int output_node,
                 size_t *out_size) {
  // Calculate blob size
  size_t blob_size = calculate_blob_size(node_defs, node_count);
  *out_size = blob_size;

  // Allocate blob memory
  void *blob = malloc(blob_size);
  if (!blob)
    return NULL;
  memset(blob, 0, blob_size);

  // Set up header
  GraphHeader *header = (GraphHeader *)blob;
  header->node_count = node_count;

  // Determine execution order
  determine_execution_order(node_defs, node_count, header->execution_order);

  // Fill in node information
  for (int i = 0; i < node_count; i++) {
    header->layouts[i] = node_defs[i].layout;
    header->input_counts[i] = node_defs[i].input_count;
    header->node_types[i] = node_defs[i].type;

    for (int j = 0; j < node_defs[i].input_count; j++) {
      header->input_indices[i][j] = node_defs[i].inputs[j];
    }
  }

  // Calculate offsets
  size_t current_offset = sizeof(GraphHeader);

  // Node data offsets
  for (int i = 0; i < node_count; i++) {
    header->node_offsets[i] = current_offset;
    current_offset += sizeof(perform_func_t) + node_defs[i].state_size;
  }

  // Output buffer offsets
  for (int i = 0; i < node_count; i++) {
    header->output_offsets[i] = current_offset;
    current_offset += node_defs[i].layout * BUF_SIZE * sizeof(double);
  }

  // Copy node data into blob
  for (int i = 0; i < node_count; i++) {
    void *node_ptr = (char *)blob + header->node_offsets[i];

    // Store perform function
    perform_func_t *fn_ptr = (perform_func_t *)node_ptr;
    *fn_ptr = get_perform_func(node_defs[i].type);

    // Copy state
    memcpy(fn_ptr + 1, node_defs[i].state, node_defs[i].state_size);
  }

  return blob;
}

// Execute packed graph
void execute_packed_graph(void *blob, int nframes, double spf, double *dac_buf,
                          int dac_layout, int output_node) {
  GraphHeader *header = (GraphHeader *)blob;

  // Temporary array to hold input pointers for each node
  double *inputs[MAX_INPUTS];

  // Process nodes in execution order
  for (int order_idx = 0; order_idx < header->node_count; order_idx++) {
    int node_idx = header->execution_order[order_idx];
    int layout = header->layouts[node_idx];

    // Get node data pointer
    void *node_ptr = (char *)blob + header->node_offsets[node_idx];
    perform_func_t perform_func = *(perform_func_t *)node_ptr;
    node_ptr += sizeof(perform_func_t);

    // Get output pointer
    double *out = (double *)((char *)blob + header->output_offsets[node_idx]);

    // Set up input pointers
    for (int i = 0; i < header->input_counts[node_idx]; i++) {
      int input_idx = header->input_indices[node_idx][i];
      inputs[i] = (double *)((char *)blob + header->output_offsets[input_idx]);
    }
    for (int i = header->input_counts[node_idx]; i < MAX_INPUTS; i++) {
      inputs[i] = NULL;
    }

    // Perform node function
    perform_func(node_ptr, layout, out, nframes, spf, inputs);
  }

  // Write output node to DAC
  double *output =
      (double *)((char *)blob + header->output_offsets[output_node]);
  int output_layout = header->layouts[output_node];

  // Write to DAC
  int dac_idx = 0;
  for (int i = 0; i < nframes; i++) {
    for (int j = 0; j < dac_layout; j++) {
      dac_buf[dac_idx++] = output[i * output_layout + (j % output_layout)];
    }
  }
}

// Free node definitions (after packing)
void free_node_defs(NodeDef *node_defs, int node_count) {
  for (int i = 0; i < node_count; i++) {
    free(node_defs[i].state);
  }
}

// Example usage
int main(int argc, char **argv) {
  // Initialize the sine table
  maketable_sin();

  // Initialize audio system
  init_audio();
  Ctx *ctx = get_audio_ctx();

  // Create node definitions
  NodeDef node_defs[MAX_NODES];
  int node_count = 0;

  // Create nodes: sine oscillator
  int sin1_idx = create_sin_node(node_defs, &node_count, 1, 220.0);  // A3
  int sin2_idx = create_sin_node(node_defs, &node_count, 1, 277.18); // C#4
  int sin3_idx = create_sin_node(node_defs, &node_count, 1, 329.63); // E4

  // Create gain nodes
  int gain1_idx = create_gain_node(node_defs, &node_count, 1, sin1_idx, 0.3);
  int gain2_idx = create_gain_node(node_defs, &node_count, 1, sin2_idx, 0.3);
  int gain3_idx = create_gain_node(node_defs, &node_count, 1, sin3_idx, 0.3);

  // Create mixer node
  int input_indices[3] = {gain1_idx, gain2_idx, gain3_idx};
  double gains[3] = {1.0, 1.0, 1.0};
  int mix_idx =
      create_mix_node(node_defs, &node_count, 2, 3, input_indices, gains);

  // Pack graph
  size_t blob_size;
  void *graph_blob = pack_graph(node_defs, node_count, mix_idx, &blob_size);

  printf("Created audio graph blob of size %zu bytes\n", blob_size);

  // Set audio callback or directly call execute_packed_graph in your audio loop

  // For demonstration, create a second instance of the graph (transposed)
  void *graph_blob2 = malloc(blob_size);
  memcpy(graph_blob2, graph_blob, blob_size);

  // Modify the second graph to transpose it up a fifth
  GraphHeader *header2 = (GraphHeader *)graph_blob2;

  // Find the sin oscillator nodes and change their frequencies
  for (int i = 0; i < header2->node_count; i++) {
    if (header2->node_types[i] == NODE_SIN) {
      SinState *state =
          (SinState *)((char *)graph_blob2 + header2->node_offsets[i] +
                       sizeof(perform_func_t));
      state->freq *= 1.5; // Up a perfect fifth
    }
  }

  // Free node definitions as they're no longer needed
  free_node_defs(node_defs, node_count);

  // Your audio loop would call execute_packed_graph for each graph

  // Clean up when done
  free(graph_blob);
  free(graph_blob2);

  return 0;
}
