#include "./glue.h"
#include "audio_graph.h"
#include "ctx.h"
#include "node.h"
#include <math.h>
// Glue compressor state structure
typedef struct {
  double threshold;    // Threshold in dB
  double ratio;        // Compression ratio
  double attack_time;  // Attack time in seconds
  double release_time; // Release time in seconds
  double makeup_gain;  // Makeup gain in dB
  double knee_width;   // Knee width in dB (for soft knee)

  // Internal state variables
  double env;            // Envelope follower
  double gain_reduction; // Current gain reduction amount
  double attack_coeff;   // Attack coefficient
  double release_coeff;  // Release coefficient

  // Optional sidechain filtering
  double sidechain_lp_state; // Sidechain low-pass filter state
  double sidechain_lp_coeff; // Sidechain low-pass filter coefficient
} glue_compressor_state;

// Initialize coefficients based on attack and release times
static void update_glue_coefficients(glue_compressor_state *state,
                                     double sample_rate) {
  // Calculate time constants
  state->attack_coeff = exp(-1.0 / (state->attack_time * sample_rate));
  state->release_coeff = exp(-1.0 / (state->release_time * sample_rate));
}

// dB to linear conversion
static double db_to_linear(double db) { return pow(10.0, db / 20.0); }

// Linear to dB conversion
static double linear_to_db(double linear) {
  return 20.0 * log10(fmax(linear, 1e-6)); // Avoid log of zero
}

// Calculate gain reduction based on input level
static double calculate_gain_reduction(double input_level_db,
                                       glue_compressor_state *state) {
  double knee_lower = state->threshold - (state->knee_width / 2.0);
  double knee_upper = state->threshold + (state->knee_width / 2.0);

  if (input_level_db < knee_lower) {
    // Below threshold, no compression
    return 0.0;
  } else if (input_level_db > knee_upper) {
    // Above threshold, full compression
    return (state->threshold - input_level_db) * (1.0 - 1.0 / state->ratio);
  } else {
    // In the knee region, gradual compression
    double knee_factor = (input_level_db - knee_lower) / state->knee_width;
    return (knee_lower - input_level_db) * (1.0 - 1.0 / state->ratio) *
           knee_factor * knee_factor;
  }
}

// Low-pass filter for sidechain signal
static double sidechain_lowpass(double input, double *state, double coeff) {
  double output = input * (1.0 - coeff) + (*state * coeff);
  *state = output;
  return output;
}

// Update compressor state and calculate gain reduction
static double process_compressor_level(double input_level_db,
                                       glue_compressor_state *state) {
  // Apply sidechain filtering if needed
  double filtered_level_db = sidechain_lowpass(
      input_level_db, &state->sidechain_lp_state, state->sidechain_lp_coeff);

  // Calculate desired gain reduction
  double target_gain_reduction =
      calculate_gain_reduction(filtered_level_db, state);

  // Apply smoothing with attack/release
  if (target_gain_reduction < state->gain_reduction) {
    // Level is increasing (attack phase)
    state->gain_reduction =
        target_gain_reduction * (1.0 - state->attack_coeff) +
        state->gain_reduction * state->attack_coeff;
  } else {
    // Level is decreasing (release phase)
    state->gain_reduction =
        target_gain_reduction * (1.0 - state->release_coeff) +
        state->gain_reduction * state->release_coeff;
  }

  return state->gain_reduction;
}

// Process a mono signal
static double process_glue_frame(double input, glue_compressor_state *state) {
  // Calculate input level in dB (abs for full-wave rectification)
  double input_abs = fabs(input);
  double input_level_db = linear_to_db(input_abs);

  // Process level and get gain reduction
  double gain_reduction = process_compressor_level(input_level_db, state);

  // Apply gain reduction and makeup gain
  double gain_linear = db_to_linear(gain_reduction + state->makeup_gain);
  return input * gain_linear;
}

// Glue compressor perform function (supports mono and stereo)
void *glue_compressor_perform(Node *node, glue_compressor_state *state,
                              Node *inputs[], int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  int channels = inputs[0]->output.layout;

  if (channels == 1) {
    // Mono processing
    while (nframes--) {
      *out = process_glue_frame(*in, state);
      in++;
      out++;
    }
  } else if (channels == 2) {
    // Stereo processing
    while (nframes--) {
      // Get left and right channel values
      double left = in[0];
      double right = in[1];

      // Find the maximum absolute value between left and right for detection
      double left_abs = fabs(left);
      double right_abs = fabs(right);
      double max_abs = (left_abs > right_abs) ? left_abs : right_abs;

      // Convert to dB for detection
      double input_level_db = linear_to_db(max_abs);

      // Process the level through the compressor
      double gain_reduction = process_compressor_level(input_level_db, state);

      // Apply the same gain reduction to both channels
      double gain_linear = db_to_linear(gain_reduction + state->makeup_gain);

      out[0] = left * gain_linear;
      out[1] = right * gain_linear;

      in += 2;
      out += 2;
    }
  } else {
    // Multi-channel processing (more than stereo)
    while (nframes--) {
      // Find maximum level across all channels
      double max_abs = 0.0;
      for (int ch = 0; ch < channels; ch++) {
        double abs_val = fabs(in[ch]);
        if (abs_val > max_abs) {
          max_abs = abs_val;
        }
      }

      // Convert to dB for detection
      double input_level_db = linear_to_db(max_abs);

      // Process the level through the compressor
      double gain_reduction = process_compressor_level(input_level_db, state);

      // Apply the same gain reduction to all channels
      double gain_linear = db_to_linear(gain_reduction + state->makeup_gain);

      for (int ch = 0; ch < channels; ch++) {
        out[ch] = in[ch] * gain_linear;
      }

      in += channels;
      out += channels;
    }
  }

  return node->output.buf;
}

// Dynamic glue compressor (with controllable threshold)
void *glue_compressor_dyn_perform(Node *node, glue_compressor_state *state,
                                  Node *inputs[], int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;
  double *threshold_in = inputs[1]->output.buf;
  int channels = inputs[0]->output.layout;

  if (channels == 1) {
    // Mono processing
    while (nframes--) {
      // Update threshold dynamically
      state->threshold = *threshold_in;

      // Process the frame
      *out = process_glue_frame(*in, state);

      in++;
      threshold_in++;
      out++;
    }
  } else if (channels == 2) {
    // Stereo processing
    while (nframes--) {
      // Update threshold dynamically
      state->threshold = *threshold_in;

      // Get left and right channel values
      double left = in[0];
      double right = in[1];

      // Find the maximum absolute value between left and right for detection
      double left_abs = fabs(left);
      double right_abs = fabs(right);
      double max_abs = (left_abs > right_abs) ? left_abs : right_abs;

      // Convert to dB for detection
      double input_level_db = linear_to_db(max_abs);

      // Process the level through the compressor
      double gain_reduction = process_compressor_level(input_level_db, state);

      // Apply the same gain reduction to both channels
      double gain_linear = db_to_linear(gain_reduction + state->makeup_gain);

      out[0] = left * gain_linear;
      out[1] = right * gain_linear;

      in += 2;
      threshold_in++; // Threshold is a mono control signal
      out += 2;
    }
  } else {
    // Multi-channel processing
    while (nframes--) {
      // Update threshold dynamically
      state->threshold = *threshold_in;

      // Find maximum level across all channels
      double max_abs = 0.0;
      for (int ch = 0; ch < channels; ch++) {
        double abs_val = fabs(in[ch]);
        if (abs_val > max_abs) {
          max_abs = abs_val;
        }
      }

      // Convert to dB for detection
      double input_level_db = linear_to_db(max_abs);

      // Process the level through the compressor
      double gain_reduction = process_compressor_level(input_level_db, state);

      // Apply the same gain reduction to all channels
      double gain_linear = db_to_linear(gain_reduction + state->makeup_gain);

      for (int ch = 0; ch < channels; ch++) {
        out[ch] = in[ch] * gain_linear;
      }

      in += channels;
      threshold_in++; // Threshold is a mono control signal
      out += channels;
    }
  }

  return node->output.buf;
}

// Create a glue compressor node
Node *glue_node(double threshold, double ratio, double attack_ms,
                double release_ms, double makeup_gain, double knee_width,
                Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(glue_compressor_state));

  // Get input channel layout
  int channels = input->output.layout;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)glue_compressor_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(glue_compressor_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(glue_compressor_state)),
      .output = (Signal){.layout = channels, // Preserve input channel layout
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph,
                                                          BUF_SIZE * channels)},
      .meta = "glue_compressor",
  };

  // Initialize state
  glue_compressor_state *state =
      (glue_compressor_state *)(graph->nodes_state_memory + node->state_offset);

  // Convert attack and release from ms to seconds
  state->threshold = threshold;
  state->ratio = ratio;
  state->attack_time = attack_ms / 1000.0;
  state->release_time = release_ms / 1000.0;
  state->makeup_gain = makeup_gain;
  state->knee_width = knee_width;

  // Initialize internal state variables
  state->env = 0.0;
  state->gain_reduction = 0.0;
  state->sidechain_lp_coeff = 0.9; // Default sidechain low-pass coefficient
  state->sidechain_lp_state = 0.0;

  // Calculate coefficients
  update_glue_coefficients(state, ctx_sample_rate());

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  node->state_ptr = state;
  return node;
}

// Create a dynamic threshold glue compressor node
Node *glue_node_dyn(Node *threshold, double ratio, double attack_ms,
                    double release_ms, double makeup_gain, double knee_width,
                    Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(glue_compressor_state));

  // Get input channel layout
  int channels = input->output.layout;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)glue_compressor_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(glue_compressor_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(glue_compressor_state)),
      .output = (Signal){.layout = channels, // Preserve input channel layout
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph,
                                                          BUF_SIZE * channels)},
      .meta = "glue_compressor_dyn",
  };

  // Initialize state
  glue_compressor_state *state =
      (glue_compressor_state *)(graph->nodes_state_memory + node->state_offset);

  // Convert attack and release from ms to seconds
  state->threshold = 0.0; // Will be set dynamically
  state->ratio = ratio;
  state->attack_time = attack_ms / 1000.0;
  state->release_time = release_ms / 1000.0;
  state->makeup_gain = makeup_gain;
  state->knee_width = knee_width;

  // Initialize internal state variables
  state->env = 0.0;
  state->gain_reduction = 0.0;
  state->sidechain_lp_coeff = 0.9; // Default sidechain low-pass coefficient
  state->sidechain_lp_state = 0.0;

  // Calculate coefficients
  update_glue_coefficients(state, ctx_sample_rate());

  // Connect inputs
  node->connections[0].source_node_index = input->node_index;
  node->connections[1].source_node_index = threshold->node_index;

  node->state_ptr = state;
  return node;
}

// Create a sidechain glue compressor with configurable sidechain low-pass
// filter
Node *glue_sc_node(double threshold, double ratio, double attack_ms,
                   double release_ms, double makeup_gain, double knee_width,
                   double sidechain_lp_freq, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(glue_compressor_state));

  // Get input channel layout
  int channels = input->output.layout;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)glue_compressor_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(glue_compressor_state),
      .state_offset =
          state_offset_ptr_in_graph(graph, sizeof(glue_compressor_state)),
      .output = (Signal){.layout = channels, // Preserve input channel layout
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph,
                                                          BUF_SIZE * channels)},
      .meta = "glue_compressor_sc",
  };

  // Initialize state
  glue_compressor_state *state =
      (glue_compressor_state *)(graph->nodes_state_memory + node->state_offset);

  // Convert attack and release from ms to seconds
  state->threshold = threshold;
  state->ratio = ratio;
  state->attack_time = attack_ms / 1000.0;
  state->release_time = release_ms / 1000.0;
  state->makeup_gain = makeup_gain;
  state->knee_width = knee_width;

  // Calculate sidechain filter coefficient based on frequency
  double sample_rate = ctx_sample_rate();
  state->sidechain_lp_coeff = exp(-2.0 * PI * sidechain_lp_freq / sample_rate);
  state->sidechain_lp_state = 0.0;

  // Initialize other internal state variables
  state->env = 0.0;
  state->gain_reduction = 0.0;

  // Calculate time coefficients
  update_glue_coefficients(state, sample_rate);

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  node->state_ptr = state;
  return node;
}
