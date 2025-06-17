#include "./filter.h"
#include "./common.h"
#include "./ctx.h"
#include "./node.h"
#include "./primes.h"
#include "audio_graph.h"
#include "node_util.h"
#include "osc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Biquad filter state
typedef struct biquad_state {
  float b0, b1, b2; // Feedforward coefficients
  float a1, a2;     // Feedback coefficients
  float x1, x2;     // Input delay elements
  float y1, y2;     // Output delay elements
  float prev_freq;  // Previous frequency value for coefficient updates
  float prev_res;   // Previous resonance value
} biquad_state;

// Initialize filter coefficients and state variables
static void set_biquad_filter_state(biquad_state *filter, float b0, float b1,
                                    float b2, float a1, float a2) {
  filter->b0 = b0;
  filter->b1 = b1;
  filter->b2 = b2;
  filter->a1 = a1;
  filter->a2 = a2;
}

// Zero filter state variables
static void zero_biquad_filter_state(biquad_state *filter) {
  filter->x1 = 0.0;
  filter->x2 = 0.0;
  filter->y1 = 0.0;
  filter->y2 = 0.0;
}

// Low-pass filter coefficient calculation
static void set_biquad_lp_coefficients(float freq, float res, int fs,
                                       biquad_state *state) {
  float fc = freq; // Cutoff frequency (Hz)
  float w0 = 2.0 * PI * fc / fs;
  float Q = res; // Quality factor

  // Compute filter coefficients
  float A = sin(w0) / (2 * Q);
  float C = cos(w0);
  float b0 = (1 - C) / 2;
  float b1 = 1 - C;
  float b2 = (1 - C) / 2;
  float a0 = 1 + A;
  float a1 = -2 * C;
  float a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Band-pass filter coefficient calculation
static void set_biquad_bp_coefficients(float freq, float res, int fs,
                                       biquad_state *state) {
  float fc = freq; // Center frequency (Hz)
  float w0 = 2.0 * PI * fc / fs;
  float Q = res; // Q factor for resonance

  // Compute filter coefficients
  float A = sin(w0) / (2 * Q);
  float C = cos(w0);
  float b0 = A;
  float b1 = 0.0;
  float b2 = -A;
  float a0 = 1 + A;
  float a1 = -2 * C;
  float a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// High-pass filter coefficient calculation
static void set_biquad_hp_coefficients(float freq, float res, int fs,
                                       biquad_state *state) {
  float fc = freq; // Cutoff frequency (Hz)
  float w0 = 2.0 * PI * fc / fs;
  float Q = res; // Quality factor

  // Compute filter coefficients
  float A = sin(w0) / (2 * Q);
  float C = cos(w0);
  float b0 = (1 + C) / 2;
  float b1 = -(1 + C);
  float b2 = (1 + C) / 2;
  float a0 = 1 + A;
  float a1 = -2 * C;
  float a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Butterworth high-pass filter coefficient calculation
static void set_butterworth_hp_coefficients(float freq, int fs,
                                            biquad_state *state) {
  float fc = freq; // Cutoff frequency (Hz)
  float w0 = 2.0 * PI * fc / fs;
  float wc = tan(w0 / 2);
  float k = wc * wc;
  float sqrt2 = sqrt(2.0);

  // Compute filter coefficients
  float b0 = 1 / (1 + sqrt2 * wc + k);
  float b1 = -2 * b0;
  float b2 = b0;
  float a1 = 2 * (k - 1) * b0;
  float a2 = (1 - sqrt2 * wc + k) * b0;

  // Initialize filter
  set_biquad_filter_state(state, b0, b1, b2, a1, a2);
}

// Biquad filter perform function
void *biquad_perform(Node *node, biquad_state *state, Node *inputs[],
                     int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;

  while (nframes--) {
    float input = *in;
    float output = state->b0 * input + state->b1 * state->x1 +
                   state->b2 * state->x2 - state->a1 * state->y1 -
                   state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;
    in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad low-pass filter perform function
void *biquad_lp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *freq_in = inputs[1]->output.buf;
  float *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  float freq = *freq_in;
  float res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_lp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_lp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    float input = *in;
    float output = state->b0 * input + state->b1 * state->x1 +
                   state->b2 * state->x2 - state->a1 * state->y1 -
                   state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad band-pass filter perform function
void *biquad_bp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *freq_in = inputs[1]->output.buf;
  float *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  float freq = *freq_in;
  float res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_bp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_bp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    float input = *in;
    float output = state->b0 * input + state->b1 * state->x1 +
                   state->b2 * state->x2 - state->a1 * state->y1 -
                   state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Dynamic biquad high-pass filter perform function
void *biquad_hp_dyn_perform(Node *node, biquad_state *state, Node *inputs[],
                            int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *freq_in = inputs[1]->output.buf;
  float *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  float freq = *freq_in;
  float res = *res_in;

  // Update coefficients if parameters changed
  if (freq != state->prev_freq || res != state->prev_res) {
    set_biquad_hp_coefficients(freq, res, (int)(1.0 / spf), state);
    state->prev_freq = freq;
    state->prev_res = res;
  }

  while (nframes--) {
    // Check if frequency or resonance changed
    freq = *freq_in;
    res = *res_in;

    if (freq != state->prev_freq || res != state->prev_res) {
      set_biquad_hp_coefficients(freq, res, (int)(1.0 / spf), state);
      state->prev_freq = freq;
      state->prev_res = res;
    }

    float input = *in;
    float output = state->b0 * input + state->b1 * state->x1 +
                   state->b2 * state->x2 - state->a1 * state->y1 -
                   state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    res_in++;
    out++;
  }

  return node->output.buf;
}

// Butterworth high-pass filter perform function
void *butterworth_hp_dyn_perform(Node *node, biquad_state *state,
                                 Node *inputs[], int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *freq_in = inputs[1]->output.buf;

  // Initial check and coefficient update
  float freq = *freq_in;

  // Update coefficients if frequency changed
  if (freq != state->prev_freq) {
    set_butterworth_hp_coefficients(freq, (int)(1.0 / spf), state);
    state->prev_freq = freq;
  }

  while (nframes--) {
    // Check if frequency changed
    freq = *freq_in;

    if (freq != state->prev_freq) {
      set_butterworth_hp_coefficients(freq, (int)(1.0 / spf), state);
      state->prev_freq = freq;
    }

    float input = *in;
    float output = state->b0 * input + state->b1 * state->x1 +
                   state->b2 * state->x2 - state->a1 * state->y1 -
                   state->a2 * state->y2;

    // Update delay elements
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    *out = output;

    in++;
    freq_in++;
    out++;
  }

  return node->output.buf;
}

// -------------------- Node Creation Functions --------------------

// Create a biquad low-pass filter node
Node *biquad_lp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_lp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_lp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return node;
}

// Create a biquad band-pass filter node
Node *biquad_bp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_bp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_bp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return node;
}

// Create a biquad high-pass filter node
Node *biquad_hp_node(Node *freq, Node *res, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)biquad_hp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "biquad_hp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return node;
}

// Create a butterworth high-pass filter node
Node *butterworth_hp_node(Node *freq, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(biquad_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)butterworth_hp_dyn_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(biquad_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(biquad_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "butterworth_hp",
  };

  // Initialize state
  biquad_state *state =
      (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);

  return node;
}

// ---------------------- Comb Filter ---------------------------

typedef struct {
  int read_pos;
  int write_pos;
  float fb;
} comb_state;

void *comb_perform(Node *node, comb_state *state, Node *inputs[], int nframes,
                   float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;

  float *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  while (nframes--) {
    // Get write and read pointers
    float *write_ptr = buf + state->write_pos;
    float *read_ptr = buf + state->read_pos;

    // Calculate output and write to buffer
    *out = *in + *read_ptr;
    *write_ptr = state->fb * (*out);

    // Update positions
    state->read_pos = (state->read_pos + 1) % bufsize;
    state->write_pos = (state->write_pos + 1) % bufsize;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input) {
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  node->state_size = sizeof(comb_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;
  state->read_pos = bufsize - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)comb_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "comb",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);

  return node;
}

// ---------------------- Dyn Comb Filter ---------------------------

void *dyn_comb_perform(Node *node, comb_state *state, Node *inputs[],
                       int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *delay_buf = inputs[1]->output.buf;
  int buf_size = inputs[1]->output.size;
  float *delay_time = inputs[2]->output.buf;
  float sample_rate = 1.0 / spf; // Calculate sample rate from seconds per frame

  while (nframes--) {
    int write_pos = state->write_pos;

    delay_buf[write_pos] = *in + (state->fb * delay_buf[state->read_pos]);

    float delay_samples = *delay_time * sample_rate;
    int read_offset = (int)delay_samples;
    float frac =
        delay_samples - read_offset; // Fractional part for interpolation

    // Ensure read position stays within buffer bounds with proper modulo
    int read_pos = (write_pos - read_offset + buf_size) % buf_size;
    int read_pos_next = (read_pos + 1) % buf_size;

    // Linear interpolation for smoother delay time changes
    float sample =
        delay_buf[read_pos] * (1.0 - frac) + delay_buf[read_pos_next] * frac;

    *out = sample;
    *out += *in;

    state->read_pos = read_pos;
    state->write_pos = (write_pos + 1) % buf_size;

    in++;
    out++;
    delay_time++;
  }

  return node->output.buf;
}

Node *dyn_comb_node(Node *delay_time, double max_delay_time, double fb,
                    Node *input) {
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  // Allocate node
  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  // Allocate state
  node->state_size = sizeof(comb_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  // Get state pointer and buffer pointer
  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)dyn_comb_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "comb",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);
  plug_input_in_graph(2, node, delay_time);

  return node;
}

// ---------------------- Lag Filter ---------------------------

typedef struct {
  float current_value;
  float target_value;
  float coeff;
  float lag_time;
} lag_state;

// Node *static_lag_node(float lag_time, Node *input) {
//   AudioGraph *graph = _graph;
//   Node *node = allocate_node_in_graph(graph, sizeof(lag_state));
//
//   // Initialize node
//   *node = (Node){
//       .perform = (perform_func_t)lag_perform,
//       .node_index = node->node_index,
//       .num_inputs = 1,
//       .state_size = sizeof(lag_state),
//       .state_offset = state_offset_ptr_in_graph(graph, sizeof(lag_state)),
//       .output = (Signal){.layout = 1,
//                          .size = BUF_SIZE,
//                          .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
//       .meta = "lag",
//   };
//
//   // Initialize state
//   lag_state *state =
//       (lag_state *)(graph->nodes_state_memory + node->state_offset);
//   state->current_value = 0.0;
//   state->target_value = 0.0;
//   state->lag_time = lag_time;
//   float spf = 1.0 / ctx_sample_rate();
//   state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));
//
//   // Connect input
//   node->connections[0].source_node_index = input->node_index;
//
//   return node;
// }
void *lag_perform(Node *node, lag_state *state, Node *inputs[], int nframes,
                  float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *lag_time = inputs[1]->output.buf;

  while (nframes--) {
    float lt = *lag_time;
    lag_time++;

    // Special case: when lag_time is 0, immediately jump to target value
    if (lt <= 0.0) {
      state->current_value = *in;
      state->coeff = 0.0; // Set coefficient to 0 to indicate immediate response
    } else {
      if (fabs(lt - state->lag_time) > 1e-6) {
        // state->lag_time = lt;
        state->coeff = exp(-1.0 / (lt * (1.0 / spf)));
      }

      // Store target value
      state->target_value = *in;

      if (lt > 0.0) {
        // Normal lag behavior: update current value with interpolation
        state->current_value =
            state->current_value +
            (state->target_value - state->current_value) * (1.0 - state->coeff);
      }
    }

    // Write output
    *out = state->current_value;

    in++;
    out++;
  }

  return node->output.buf;
}

Node *lag_node(NodeRef lag_time, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(lag_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)lag_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = sizeof(lag_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(lag_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "lag",
  };

  // Initialize state
  lag_state *state =
      (lag_state *)(graph->nodes_state_memory + node->state_offset);
  state->current_value = 0.0;
  state->target_value = 0.0;
  // state->lag_time = 0.0;
  state->coeff = 0.0; // Initialize coefficient to 0

  // Connect input
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, lag_time);

  return node;
}

// ---------------------- Tanh Distortion ---------------------------

typedef struct {
  float gain;
} tanh_state;

void *tanh_perform(Node *node, tanh_state *state, Node *inputs[], int nframes,
                   float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;

  float gain = state->gain;
  // printf("tanh perform %f\n", gain);

  while (nframes--) {
    *out = tanhf(*in * gain);
    in++;
    out++;
  }

  return node->output.buf;
}

Node *tanh_node(double gain, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, sizeof(tanh_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)tanh_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(tanh_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(tanh_state)),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "tanh",
  };

  // Initialize state
  tanh_state *state =
      (tanh_state *)(graph->nodes_state_memory + node->state_offset);
  state->gain = gain;

  // Connect input
  plug_input_in_graph(0, node, input);

  return node;
}

void *dyn_tanh_perform(Node *node, tanh_state *state, Node *inputs[],
                       int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  float *gain = inputs[1]->output.buf;

  while (nframes--) {

    *out = tanh(*in * *gain);
    gain++;
    in++;
    out++;
  }

  return node->output.buf;
}

Node *dyn_tanh_node(NodeRef gain, Node *input) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)dyn_tanh_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = state_offset_ptr_in_graph(graph, 0),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "tanh",
  };

  // Connect input
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, gain);

  //
  return node;
}

typedef struct {
  int length;
  int pos;
  float coeff;
} allpass_state;

float process_allpass_frame(float *buf, int length, int *pos, float coeff,
                            float in) {
  int bufsize = length;
  float delayed = buf[*pos];
  buf[*pos] = in;
  *pos = (*pos + 1) % bufsize;
  return delayed - coeff * in;
}

void *allpass_perform(Node *node, allpass_state *state, Node *inputs[],
                      int nframes, float spf) {
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  char *mem = state + 1;
  float *buf = mem;

  while (nframes--) {
    *out = process_allpass_frame(buf, state->length, &state->pos, state->coeff,
                                 *in);
    in++;
    out++;
  }

  return node->output.buf;
}

static int compute_allpass_delay_length(float sample_rate, float delay_sec) {
  int delay_samples = (int)(delay_sec * sample_rate);
  return delay_samples;
}

Node *allpass_node(double time, double coeff, Node *input) {
  AudioGraph *graph = _graph;
  int state_size = sizeof(allpass_state);
  allpass_state ap = {
      .length = compute_allpass_delay_length(ctx_sample_rate(), time),
      .pos = 0,
      .coeff = coeff,
  };

  state_size += (sizeof(float) * ap.length); // leave space for buffer

  Node *node = allocate_node_in_graph(graph, sizeof(allpass_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)allpass_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "ap",
  };

  // Initialize state
  allpass_state *state =
      (allpass_state *)(graph->nodes_state_memory + node->state_offset);
  *state = ap;

  // Connect input
  plug_input_in_graph(0, node, input);

  return node;
}

#define MAX_DELAY_LENGTH 20000
#define NUM_EARLY_REFLECTIONS 8
#define NUM_ALLPASS 4

static float early_gains[NUM_EARLY_REFLECTIONS] = {1.0, 0.9, 0.8, 0.7,
                                                   0.6, 0.5, 0.4, 0.3};
static float early_scaling_factors[NUM_EARLY_REFLECTIONS] = {
    1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7};
static float early_gain_factors[NUM_EARLY_REFLECTIONS] = {1.0, 0.9, 0.8, 0.7,
                                                          0.6, 0.5, 0.4, 0.3};

static float delay_scaling_factors[4] = {1.0, 0.9, 0.8, 0.7};
static float allpass_scaling_factors[NUM_ALLPASS] = {0.15, 0.12, 0.10, 0.08};

static int next_prime(int n) {
  int i = 0;
  while (PRIMES[i] <= n && i < NUM_PRIMES) {
    i++;
  }
  return PRIMES[i];
}

static int compute_delay_length(float room_size, float sample_rate,
                                float scaling_factor) {
  float delay_sec = (room_size * scaling_factor) / 343.0f;
  int delay_samples = (int)(delay_sec * sample_rate);
  return next_prime(delay_samples);
}

typedef struct GVerb {
  float room_size;
  float reverb_time;
  float damping;
  float input_bandwidth;
  float dry;
  float wet;
  float early_level;
  float tail_level;
  float sample_rate;

  float input_lp_state;
  float input_lp_coeff;
  int early_lengths[NUM_EARLY_REFLECTIONS];
  int early_pos[NUM_EARLY_REFLECTIONS];
  float early_gains[NUM_EARLY_REFLECTIONS];
  int fb_delay_lengths[4];
  int fb_delay_pos[4];
  float damping_coeff;
  float damping_state[4];
  int allpass_lengths[NUM_ALLPASS];
  int allpass_pos[NUM_ALLPASS];
  float allpass_feedback;
} GVerb;

static float lowpass_filter(float input, float *state, float coeff) {
  float output = input * (1.0f - coeff) + (*state * coeff);
  *state = output;
  return output;
}

static float process_early_reflections(GVerb *refl, float **delays,
                                       float input) {

  float early_out = 0.;
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    int bufsize = refl->early_lengths[i];
    float *buf = delays[i];
    int write_pos = refl->early_pos[i] % bufsize;
    early_out += process_allpass_frame(delays[i], bufsize, &refl->early_pos[i],
                                       -1., input);
  }
  return early_out;
}

static float series_allpass_process(GVerb *gverb, float **allpass_delays,
                                    float input) {

  for (int i = 0; i < NUM_ALLPASS; i++) {
    float *delay_line = allpass_delays[i];
    int pos = gverb->allpass_pos[i];
    int length = gverb->allpass_lengths[i];
    input = process_allpass_frame(delay_line, length,
                                  &gverb->allpass_lengths[i], 0.0, input);
  }
  return input;
}

float feedback_delay_network(GVerb *gverb, float **delay_lines, float mix) {
  float delay_out[4];
  for (int i = 0; i < 4; i++) {
    int fb_pos = gverb->fb_delay_pos[i];
    delay_out[i] = delay_lines[i][fb_pos];
  }

  for (int i = 0; i < 4; i++) {
    delay_out[i] = lowpass_filter(delay_out[i], &gverb->damping_state[i],
                                  gverb->damping_coeff);
  }

  float feedback[4];
  feedback[0] = delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3];
  feedback[1] = delay_out[0] + delay_out[1] - delay_out[2] - delay_out[3];
  feedback[2] = delay_out[0] - delay_out[1] + delay_out[2] - delay_out[3];
  feedback[3] = delay_out[0] - delay_out[1] - delay_out[2] + delay_out[3];

  float rt_gain = pow(10.0, -3.0 * (1.0 / gverb->reverb_time));

  for (int i = 0; i < 4; i++) {
    feedback[i] *= 0.25 * rt_gain; /* 0.25 for Hadamard normalization */
  }

  feedback[0] += mix;

  for (int i = 0; i < 4; i++) {
    int pos = gverb->fb_delay_pos[i];
    int len = gverb->fb_delay_lengths[i];
    delay_lines[i][pos] = feedback[i];
    gverb->fb_delay_pos[i] = (pos + 1) % len;
  }
  return delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3];
}

static float allpass_process(float input, float *delay_line, int *pos,
                             int length, float feedback) {
  float delayed = delay_line[*pos];
  float new_input = input + feedback * delayed;
  delay_line[*pos] = new_input;
  *pos = (*pos + 1) % length;
  return delayed - feedback * new_input;
}

void *gverb_perform(Node *node, GVerb *gverb, Node *inputs[], int nframes,
                    float spf) {

  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  char *mem = ((GVerb *)gverb + 1);

  float *early_delays[NUM_EARLY_REFLECTIONS];
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    early_delays[i] = mem;
    mem += (sizeof(float) * gverb->early_lengths[i]);
  }

  float *delay_lines[4];
  for (int i = 0; i < 4; i++) {
    delay_lines[i] = mem;
    mem += (sizeof(float) * gverb->fb_delay_lengths[i]);
  }

  float *allpass_delays[NUM_ALLPASS];
  for (int i = 0; i < NUM_ALLPASS; i++) {
    allpass_delays[i] = mem;
    mem += (sizeof(float) * gverb->allpass_lengths[i]);
  }

  while (nframes--) {
    float filt =
        lowpass_filter(*in, &gverb->input_lp_state, gverb->input_lp_coeff);

    float early_out = process_early_reflections(gverb, early_delays, filt);

    float mix = filt * 0.5 + early_out * 0.5;

    float diffuse_signal = feedback_delay_network(gverb, delay_lines, mix);

    diffuse_signal *= 0.25;

    for (int i = 0; i < NUM_ALLPASS; i++) {
      float *delay_line = allpass_delays[i];
      int pos = gverb->allpass_pos[i];
      int length = gverb->allpass_lengths[i];

      diffuse_signal = allpass_process(diffuse_signal, delay_line, &pos, length,
                                       gverb->allpass_feedback);
      gverb->allpass_pos[i] = pos;
    }

    *out = *in * gverb->dry + early_out * gverb->early_level * gverb->wet +
           diffuse_signal * gverb->tail_level * gverb->wet;
    in++;
    out++;
  }
}

Node *gverb_node(Node *input) {
  float sr = (float)ctx_sample_rate();
  float room_size = 10.;
  GVerb gverb = {
      .room_size = room_size,
      .reverb_time = 10.,
      .damping_coeff = 0.4,
      .input_lp_coeff = 0.4,
      .dry = 0.5,
      .wet = 0.5,
      .early_level = 0.5,
      .tail_level = 0.9,
      .sample_rate = sr,
      .early_pos = {0, 0, 0, 0, 0, 0, 0, 0},
      .early_gains = {1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3},
      .early_lengths =
          {
              compute_delay_length(room_size, sr, 1.0),
              compute_delay_length(room_size, sr, 1.1),
              compute_delay_length(room_size, sr, 1.2),
              compute_delay_length(room_size, sr, 1.3),
              compute_delay_length(room_size, sr, 1.4),
              compute_delay_length(room_size, sr, 1.5),
              compute_delay_length(room_size, sr, 1.6),
              compute_delay_length(room_size, sr, 1.7),
          },
      .fb_delay_pos = {0, 0, 0, 0},
      .fb_delay_lengths =
          {
              compute_delay_length(room_size, sr, delay_scaling_factors[0]),
              compute_delay_length(room_size, sr, delay_scaling_factors[1]),
              compute_delay_length(room_size, sr, delay_scaling_factors[2]),
              compute_delay_length(room_size, sr, delay_scaling_factors[3]),
          },
      .allpass_pos = {0, 0, 0, 0},
      .allpass_lengths =
          {
              compute_delay_length(room_size, sr, allpass_scaling_factors[0]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[1]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[2]),
              compute_delay_length(room_size, sr, allpass_scaling_factors[3]),
          },

  };

  AudioGraph *graph = _graph;
  int state_size = sizeof(GVerb);
  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    state_size += (gverb.early_lengths[i] * sizeof(float));
  }

  for (int i = 0; i < 4; i++) {
    state_size += (gverb.fb_delay_lengths[i] * sizeof(float));
  }

  for (int i = 0; i < NUM_ALLPASS; i++) {
    state_size += (gverb.allpass_lengths[i] * sizeof(float));
  }

  state_size = (state_size + 7) & ~7;
  Node *node = allocate_node_in_graph(graph, state_size);
  *node = (Node){
      .perform = (perform_func_t)gverb_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "gverb_lp",
  };

  char *mem = graph->nodes_state_memory + node->state_offset;
  memset(mem, 0, state_size);
  GVerb *state = (GVerb *)(mem);
  *state = gverb;
  plug_input_in_graph(0, node, input);

  return node;
}
typedef struct grain_pitchshift_state {
  int length;
  int pos;
  int max_grains;
  int active_grains;
  int next_trig;
  int trig_gap_in_frames;
  float width;
  float rate;
  float fb;
} grain_pitchshift_state;

float __pow2table_read(float pos, int tabsize, float *table) {
  int mask = tabsize - 1;

  float env_pos = pos * (mask);
  int env_idx = (int)env_pos;
  float env_frac = env_pos - env_idx;

  // Interpolate between envelope table values
  float env_val = table[env_idx & mask] * (1.0 - env_frac) +
                  table[(env_idx + 1) & mask] * env_frac;
  return env_val;
}

void *granular_pitchshift_perform(Node *node, grain_pitchshift_state *state,
                                  Node *inputs[], int nframes, float spf) {

  int out_layout = node->output.layout;
  float *out = node->output.buf;
  float *in = inputs[0]->output.buf;
  char *mem = state + 1;
  float *buf = mem;
  int buf_size = state->length;
  mem += buf_size * sizeof(float);

  int max_grains = state->max_grains;

  float *phases = (float *)mem;
  mem += sizeof(float) * max_grains;

  float *widths = (float *)mem;
  mem += sizeof(float) * max_grains;

  float *remaining_secs = (float *)mem;
  mem += sizeof(float) * max_grains;

  float *starts = (float *)mem;
  mem += sizeof(float) * max_grains;

  int *active = (int *)mem;

  float d_index;
  int index;
  float frac;
  float a, b;
  float sample = 0.;
  float r = state->rate;
  const int table_mask = GRAIN_WINDOW_TABSIZE - 1;
  while (nframes--) {
    sample = 0.;
    if ((state->next_trig <= 0) && state->active_grains < max_grains) {
      state->next_trig = state->trig_gap_in_frames;
      for (int i = 0; i < max_grains; i++) {
        if (active[i] == 0) {
          phases[i] = 0;
          starts[i] = state->pos * buf_size;
          widths[i] = state->width;
          remaining_secs[i] = state->width;
          active[i] = 1;
          state->active_grains++;
          break;
        }
      }
    }

    for (int i = 0; i < max_grains; i++) {

      if (active[i]) {
        float p = phases[i];
        float s = starts[i];
        float w = widths[i];
        float rem = remaining_secs[i];

        d_index = s + (p * buf_size);

        index = (int)d_index;
        frac = d_index - index;

        a = buf[index % buf_size];
        b = buf[(index + 1) % buf_size];

        float grain_elapsed = 1.0 - (rem / w);
        float env_val =
            __pow2table_read(grain_elapsed, GRAIN_WINDOW_TABSIZE, grain_win);

        sample += env_val * ((1.0 - frac) * a + (frac * b));
        phases[i] += (r / buf_size);

        remaining_secs[i] -= spf;
        if (remaining_secs[i] <= 0) {
          active[i] = 0; // Deactivate the grain
          state->active_grains--;
        }
      }
    }

    for (int i = 0; i < out_layout; i++) {
      *out = sample + *in;
      out++;
    }
    buf[state->pos] = *in + state->fb * *out;
    state->next_trig--;
    state->pos = (state->pos + 1) % buf_size;
  }
}

NodeRef grain_pitchshift_node(double shift, double fb, NodeRef input) {
  AudioGraph *graph = _graph;

  int max_grains = 32;
  int state_size = sizeof(grain_pitchshift_state) +
                   (1024 * sizeof(float))         // delay buffer
                   + (max_grains * sizeof(float)) // phases
                   + (max_grains * sizeof(float)) // widths
                   + (max_grains * sizeof(float)) // elapsed
                   + (max_grains * sizeof(float)) // starts
                   + (max_grains * sizeof(int))   // active grains
      ;

  grain_pitchshift_state pshift = {

      .length = 1024,
      .pos = 0,
      .max_grains = max_grains,
      .active_grains = 0,
      .next_trig = ((float)1024) / shift,
      .trig_gap_in_frames = ((float)1024) / shift,
      .width = 0.01,
      .rate = shift,
      .fb = fb};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)granular_pitchshift_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "grain_pitchshift",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  grain_pitchshift_state *state = mem;
  *state = pshift;

  plug_input_in_graph(0, node, input);

  return node;
}

typedef double (*node_math_func_t)(double samp);
typedef struct math_node_state {
  node_math_func_t math_fn;
} math_node_state;

void *math_perform(Node *node, math_node_state *state, Node *inputs[],
                   int nframes, float spf) {
  float *out = node->output.buf;
  Signal in = inputs[0]->output;

  for (int i = 0; i < in.size * in.layout; i++) {
    out[i] = state->math_fn(in.buf[i]);
  }

  return node->output.buf;
}

NodeRef math_node(MathNodeFn math_fn, NodeRef input) {
  AudioGraph *graph = _graph;

  int state_size = sizeof(math_node_state);

  math_node_state m = {.math_fn = math_fn};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)math_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = input->output.layout,
                         .size = input->output.size,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * input->output.size)},
      .meta = "node_math",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  math_node_state *state = mem;
  *state = m;

  // node->connections[0].source_node_index = input->node_index;
  plug_input_in_graph(0, node, input);
  //
  return node;
}

// typedef struct stutter_state {
//   int buf_chans;
//   int buf_size;
//   int write_head;
//   int is_stuttering;  // Flag to track if we're in stutter mode
//   int stutter_start;  // Position to start reading from for stutter
// } stutter_state;
typedef struct stutter_state {
  int buf_chans;
  int buf_size;
  int write_head;
  int is_stuttering;
  int stutter_start;
  int stutter_position; // Current position within stutter section
  int stutter_counter;  // Counter for creating rhythmic stutters
} stutter_state;

void *stutter_perform(Node *node, stutter_state *state, Node *inputs[],
                      int nframes, float spf) {
  float *buf_memory = (float *)((stutter_state *)state + 1);

  Signal in = inputs[0]->output;
  float *_gate = inputs[1]->output.buf;
  float *_repeat_time = inputs[2]->output.buf;
  float *out = node->output.buf;

  int chans = state->buf_chans;
  int sample_rate = (int)(1.0 / spf);

  for (int i = 0; i < nframes; i++) {
    float gate = *_gate;
    _gate++;
    float repeat_time = *_repeat_time;
    _repeat_time++;

    // Calculate frames to repeat (smaller for more rapid stuttering)
    int repeat_frames = (int)(repeat_time * sample_rate);
    if (repeat_frames > state->buf_size)
      repeat_frames = state->buf_size;
    if (repeat_frames < 1)
      repeat_frames = 1;

    // Stutter division - divide repeat_time into smaller chunks for rapid
    // stuttering
    int stutter_chunk_size = repeat_frames / 8; // Divide into 8 small chunks
    if (stutter_chunk_size < 1)
      stutter_chunk_size = 1;

    // Gate logic: detect gate changes
    if (gate > 0.5) {
      if (!state->is_stuttering) {
        // Start stuttering - capture current audio
        state->is_stuttering = 1;
        state->stutter_start = state->write_head - repeat_frames;
        if (state->stutter_start < 0)
          state->stutter_start += state->buf_size;
        state->stutter_position = state->stutter_start;
        state->stutter_counter = 0;
      }
    } else {
      state->is_stuttering = 0;
    }

    // Write input to buffer regardless of stutter state
    for (int c = 0; c < chans; c++) {
      buf_memory[chans * state->write_head + c] = in.buf[(i * chans) + c];
    }

    // Process output samples
    for (int j = 0; j < chans; j++) {
      if (state->is_stuttering) {
        // Read from current stutter position
        out[i * chans + j] = buf_memory[chans * state->stutter_position + j];
      } else {
        // Pass through input
        out[i * chans + j] = in.buf[i * chans + j];
      }
    }

    // Update write head
    // if (!state->is_stuttering) {
    state->write_head++;
    if (state->write_head >= state->buf_size) {
      state->write_head = 0;
    }
    // }

    // Update stutter playback position for rapid stutter effect
    if (state->is_stuttering) {
      state->stutter_counter++;

      // Reset counter and jump to next chunk when we finish current chunk
      if (state->stutter_counter >= stutter_chunk_size) {
        state->stutter_counter = 0;

        state->stutter_position = state->stutter_start;

        // Ensure we're within buffer bounds
        if (state->stutter_position >= state->buf_size) {
          state->stutter_position -= state->buf_size;
        }
      } else {
        // Within a chunk, just increment normally
        state->stutter_position++;
        if (state->stutter_position >= state->buf_size) {
          state->stutter_position = 0;
        }
      }
    }
  }
  return node->output.buf;
}

NodeRef stutter_node(double max_time, NodeRef repeat_time, NodeRef gate,
                     NodeRef input) {
  AudioGraph *graph = _graph;

  int state_size = sizeof(stutter_state); // Fixed: was using math_node_state
  int sample_rate = ctx_sample_rate();
  int in_chans = input->output.layout;
  stutter_state s = {.buf_chans = in_chans,
                     .buf_size = sample_rate * max_time,
                     .write_head = 0,
                     .is_stuttering = 0,
                     .stutter_start = 0};

  int required_buf_frames = s.buf_chans * s.buf_size;
  state_size += required_buf_frames * sizeof(float);

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)stutter_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = input->output.layout,
                         .size = input->output.size,
                         .buf = allocate_buffer_from_pool(
                             graph, in_chans * input->output.size)},
      .meta = "stutter",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);

  memset(mem, 0, state_size);
  stutter_state *state = mem;
  *state = s;

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, gate);
  plug_input_in_graph(2, node, repeat_time);
  return node;
}
