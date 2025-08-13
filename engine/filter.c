#include "filter.h"
#include "./common.h"
#include "./ctx.h"
#include "./filter_ext.h"
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
  sample_t b0, b1, b2; // Feedforward coefficients
  sample_t a1, a2;     // Feedback coefficients
  sample_t x1, x2;     // Input delay elements
  sample_t y1, y2;     // Output delay elements
  sample_t prev_freq;  // Previous frequency value for coefficient updates
  sample_t prev_res;   // Previous resonance value
} biquad_state;

// Initialize filter coefficients and state variables
static void set_biquad_filter_state(biquad_state *filter, sample_t b0,
                                    sample_t b1, sample_t b2, sample_t a1,
                                    sample_t a2) {
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
static void set_biquad_lp_coefficients(sample_t freq, sample_t res, int fs,
                                       biquad_state *state) {
  sample_t fc = freq; // Cutoff frequency (Hz)
  sample_t w0 = 2.0 * PI * fc / fs;
  sample_t Q = res; // Quality factor

  // Compute filter coefficients
  sample_t A = sinf(w0) / (2 * Q);
  sample_t C = cosf(w0);
  sample_t b0 = (1 - C) / 2;
  sample_t b1 = 1 - C;
  sample_t b2 = (1 - C) / 2;
  sample_t a0 = 1 + A;
  sample_t a1 = -2 * C;
  sample_t a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Band-pass filter coefficient calculation
static void set_biquad_bp_coefficients(sample_t freq, sample_t res, int fs,
                                       biquad_state *state) {
  sample_t fc = freq; // Center frequency (Hz)
  sample_t w0 = 2.0 * PI * fc / fs;
  sample_t Q = res; // Q factor for resonance

  // Compute filter coefficients
  sample_t A = sinf(w0) / (2 * Q);
  sample_t C = cosf(w0);
  sample_t b0 = A;
  sample_t b1 = 0.0;
  sample_t b2 = -A;
  sample_t a0 = 1 + A;
  sample_t a1 = -2 * C;
  sample_t a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// High-pass filter coefficient calculation
static void set_biquad_hp_coefficients(sample_t freq, sample_t res, int fs,
                                       biquad_state *state) {
  sample_t fc = freq; // Cutoff frequency (Hz)
  sample_t w0 = 2.0 * PI * fc / fs;
  sample_t Q = res; // Quality factor

  // Compute filter coefficients
  sample_t A = sinf(w0) / (2 * Q);
  sample_t C = cosf(w0);
  sample_t b0 = (1 + C) / 2;
  sample_t b1 = -(1 + C);
  sample_t b2 = (1 + C) / 2;
  sample_t a0 = 1 + A;
  sample_t a1 = -2 * C;
  sample_t a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

// Butterworth high-pass filter coefficient calculation
static void set_butterworth_hp_coefficients(sample_t freq, int fs,
                                            biquad_state *state) {
  sample_t fc = freq; // Cutoff frequency (Hz)
  sample_t w0 = 2.0 * PI * fc / fs;
  sample_t wc = tan(w0 / 2);
  sample_t k = wc * wc;
  sample_t sqrt2 = sqrt(2.0);

  // Compute filter coefficients
  sample_t b0 = 1 / (1 + sqrt2 * wc + k);
  sample_t b1 = -2 * b0;
  sample_t b2 = b0;
  sample_t a1 = 2 * (k - 1) * b0;
  sample_t a2 = (1 - sqrt2 * wc + k) * b0;

  // Initialize filter
  set_biquad_filter_state(state, b0, b1, b2, a1, a2);
}

// Biquad filter perform function
void *biquad_perform(Node *node, biquad_state *state, Node *inputs[],
                     int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;

  while (nframes--) {
    sample_t input = *in;
    sample_t output = state->b0 * input + state->b1 * state->x1 +
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
                            int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *freq_in = inputs[1]->output.buf;
  sample_t *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  sample_t freq = *freq_in;
  sample_t res = *res_in;

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

    sample_t input = *in;
    sample_t output = state->b0 * input + state->b1 * state->x1 +
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
                            int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *freq_in = inputs[1]->output.buf;
  sample_t *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  sample_t freq = *freq_in;
  sample_t res = *res_in;

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

    sample_t input = *in;
    sample_t output = state->b0 * input + state->b1 * state->x1 +
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
                            int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *freq_in = inputs[1]->output.buf;
  sample_t *res_in = inputs[2]->output.buf;

  // Initial check and coefficient update
  sample_t freq = *freq_in;
  sample_t res = *res_in;

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

    sample_t input = *in;
    sample_t output = state->b0 * input + state->b1 * state->x1 +
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
                                 Node *inputs[], int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *freq_in = inputs[1]->output.buf;

  // Initial check and coefficient update
  sample_t freq = *freq_in;

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

    sample_t input = *in;
    sample_t output = state->b0 * input + state->b1 * state->x1 +
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
  biquad_state *state = state_ptr(graph, node);
  // (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return graph_embed(node);
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
  biquad_state *state = state_ptr(graph, node);

  // (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return graph_embed(node);
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
  biquad_state *state = state_ptr(graph, node);
  // (biquad_state *)(graph->nodes_state_memory + node->state_offset);
  zero_biquad_filter_state(state);
  state->prev_freq = 0.0;
  state->prev_res = 0.0;

  // Connect inputs
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, freq);
  plug_input_in_graph(2, node, res);

  return graph_embed(node);
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

  return graph_embed(node);
}

// ---------------------- Delay Filter ---------------------------

typedef struct {
  int read_pos;
  int write_pos;
  sample_t fb;
} delay_state;

void *delay_perform(Node *node, delay_state *state, Node *inputs[], int nframes,
                    sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  // Calculate delay buffer size per channel
  int delay_per_channel = bufsize / in_layout;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      // Calculate buffer offset for this channel
      int channel_offset = ch * delay_per_channel;

      // Get write and read pointers for this channel
      sample_t *write_ptr = buf + channel_offset + state->write_pos;
      sample_t *read_ptr = buf + channel_offset + state->read_pos;

      // Calculate output and write to buffer
      *out = *in + *read_ptr;
      *write_ptr = state->fb * (*out);

      in++;
      out++;
    }

    // Update positions once per frame (after processing all channels)
    state->read_pos = (state->read_pos + 1) % delay_per_channel;
    state->write_pos = (state->write_pos + 1) % delay_per_channel;
  }

  return node->output.buf;
}

Node *delay_node(double delay_time, double max_delay_time, double fb,
                 Node *input) {
  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(delay_state));

  node->state_size = sizeof(delay_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  delay_state *state =
      (delay_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;
  state->read_pos = bufsize - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)delay_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * BUF_SIZE)},
      .meta = "delay",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);

  return graph_embed(node);
}
typedef struct {
  int read_pos_left;
  int write_pos_left;
  int read_pos_right;
  int write_pos_right;
  sample_t fb;
} delay2_state;

void *delay2_perform(Node *node, delay2_state *state, Node *inputs[],
                     int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  // Calculate delay buffer size per channel
  int delay_per_channel = bufsize / in_layout;

  while (nframes--) {
    if (in_layout >= 1) {
      // Left channel (channel 0)
      int channel_offset = 0 * delay_per_channel;
      sample_t *write_ptr = buf + channel_offset + state->write_pos_left;
      sample_t *read_ptr = buf + channel_offset + state->read_pos_left;

      *out = *in + *read_ptr;
      *write_ptr = state->fb * (*out);

      in++;
      out++;
    }

    if (in_layout >= 2) {
      // Right channel (channel 1)
      int channel_offset = 1 * delay_per_channel;
      sample_t *write_ptr = buf + channel_offset + state->write_pos_right;
      sample_t *read_ptr = buf + channel_offset + state->read_pos_right;

      *out = *in + *read_ptr;
      *write_ptr = state->fb * (*out);

      in++;
      out++;
    }

    // Handle additional channels with right channel timing (if any)
    for (int ch = 2; ch < in_layout; ch++) {
      int channel_offset = ch * delay_per_channel;
      sample_t *write_ptr = buf + channel_offset + state->write_pos_right;
      sample_t *read_ptr = buf + channel_offset + state->read_pos_right;

      *out = *in + *read_ptr;
      *write_ptr = state->fb * (*out);

      in++;
      out++;
    }

    // Update positions once per frame - each channel has independent timing
    state->read_pos_left = (state->read_pos_left + 1) % delay_per_channel;
    state->write_pos_left = (state->write_pos_left + 1) % delay_per_channel;

    state->read_pos_right = (state->read_pos_right + 1) % delay_per_channel;
    state->write_pos_right = (state->write_pos_right + 1) % delay_per_channel;
  }

  return node->output.buf;
}

Node *delay2_node(double delay_time_left, double delay_time_right,
                  double max_delay_time, double fb, Node *input) {

  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(delay2_state));

  node->state_size = sizeof(delay2_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  delay2_state *state =
      (delay2_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;

  // Calculate delay buffer size per channel
  int delay_per_channel = bufsize / in_layout;

  // Initialize left channel positions
  state->write_pos_left = 0;
  state->read_pos_left =
      delay_per_channel - (int)(delay_time_left * sample_rate);
  if (state->read_pos_left < 0)
    state->read_pos_left += delay_per_channel;

  // Initialize right channel positions
  state->write_pos_right = 0;
  state->read_pos_right =
      delay_per_channel - (int)(delay_time_right * sample_rate);
  if (state->read_pos_right < 0)
    state->read_pos_right += delay_per_channel;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)delay2_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * BUF_SIZE)},
      .meta = "delay2",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);

  return graph_embed(node);
}

// Convenience function for creating stereo delay with slight time differences
Node *stereo_spread_delay_node(double delay_time, double spread_amount,
                               double max_delay_time, double fb, Node *input) {
  sample_t delay_left = delay_time - spread_amount * 0.5;
  sample_t delay_right = delay_time + spread_amount * 0.5;

  // Ensure delays are positive
  if (delay_left < 0.0)
    delay_left = 0.0;
  if (delay_right < 0.0)
    delay_right = 0.0;

  return delay2_node(delay_left, delay_right, max_delay_time, fb, input);
}

// ---------------------- Dyn delay Filter ---------------------------

void *__dyn_delay_perform(Node *node, delay_state *state, Node *inputs[],
                          int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *delay_buf = inputs[1]->output.buf;
  int buf_size = inputs[1]->output.size;
  sample_t *delay_time = inputs[2]->output.buf;
  sample_t sample_rate =
      1.0 / spf; // Calculate sample rate from seconds per frame

  while (nframes--) {
    int write_pos = state->write_pos;

    delay_buf[write_pos] = *in + (state->fb * delay_buf[state->read_pos]);

    sample_t delay_samples = *delay_time * sample_rate;
    int read_offset = (int)delay_samples;
    sample_t frac =
        delay_samples - read_offset; // Fractional part for interpolation

    // Ensure read position stays within buffer bounds with proper modulo
    int read_pos = (write_pos - read_offset + buf_size) % buf_size;
    int read_pos_next = (read_pos + 1) % buf_size;

    // Linear interpolation for smoother delay time changes
    sample_t sample =
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

void *dyn_delay_perform(Node *node, delay_state *state, Node *inputs[],
                        int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *delay_buf = inputs[1]->output.buf;
  int buf_size = inputs[1]->output.size;
  sample_t *delay_time = inputs[2]->output.buf;
  sample_t sample_rate =
      1.0 / spf; // Calculate sample rate from seconds per frame

  // Calculate delay buffer size per channel
  int delay_per_channel = buf_size / in_layout;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      // Calculate buffer offset for this channel
      int channel_offset = ch * delay_per_channel;

      int write_pos = state->write_pos;
      int write_pos_abs = channel_offset + write_pos;

      // Write input + feedback to delay buffer
      delay_buf[write_pos_abs] =
          *in + (state->fb * delay_buf[channel_offset + state->read_pos]);

      // Calculate delay in samples and interpolation
      sample_t delay_samples = *delay_time * sample_rate;
      int read_offset = (int)delay_samples;
      sample_t frac =
          delay_samples - read_offset; // Fractional part for interpolation

      // Ensure read position stays within buffer bounds with proper modulo
      int read_pos =
          (write_pos - read_offset + delay_per_channel) % delay_per_channel;
      int read_pos_next = (read_pos + 1) % delay_per_channel;

      // Add channel offset to get absolute positions
      int read_pos_abs = channel_offset + read_pos;
      int read_pos_next_abs = channel_offset + read_pos_next;

      // Linear interpolation for smoother delay time changes
      sample_t sample = delay_buf[read_pos_abs] * (1.0 - frac) +
                        delay_buf[read_pos_next_abs] * frac;

      *out = sample + *in;

      in++;
      out++;
    }

    // Update positions once per frame (after processing all channels)
    state->write_pos = (state->write_pos + 1) % delay_per_channel;
    // Note: read_pos is calculated dynamically based on delay_time, so we don't
    // update it here

    delay_time++;
  }

  return node->output.buf;
}

Node *dyn_delay_node(Node *delay_time, double max_delay_time, double fb,
                     Node *input) {
  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  // Allocate node
  Node *node = allocate_node_in_graph(graph, sizeof(delay_state));

  // Allocate state
  node->state_size = sizeof(delay_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  // Get state pointer and buffer pointer
  delay_state *state =
      (delay_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb;
  state->write_pos = 0;

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)dyn_delay_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = in_layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, in_layout *
                                                                     BUF_SIZE)},
      .meta = "delay",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);
  plug_input_in_graph(2, node, delay_time);

  return graph_embed(node);
}

typedef struct {
  int read_pos;
  int write_pos;
  sample_t fb;
  sample_t ff; // feedforward gain
} comb_state;

void *comb_perform(Node *node, comb_state *state, Node *inputs[], int nframes,
                   sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  // Calculate delay buffer size per channel
  int delay_per_channel = bufsize / in_layout;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      // Calculate buffer offset for this channel
      int channel_offset = ch * delay_per_channel;

      // Get write and read pointers for this channel
      sample_t *write_ptr = buf + channel_offset + state->write_pos;
      sample_t *read_ptr = buf + channel_offset + state->read_pos;

      // True comb filter: y[n] = x[n] + ff * x[n-M] + fb * y[n-M]
      // where read_ptr points to x[n-M] and y[n-M] from previous iterations
      sample_t delayed_signal = *read_ptr;

      // Calculate output: input + feedforward*delayed_input +
      // feedback*delayed_output
      *out = *in + state->ff * delayed_signal;

      // Write input to delay buffer for feedforward path
      *write_ptr = *in + state->fb * delayed_signal;

      in++;
      out++;
    }

    // Update positions once per frame (after processing all channels)
    state->read_pos = (state->read_pos + 1) % delay_per_channel;
    state->write_pos = (state->write_pos + 1) % delay_per_channel;
  }

  return node->output.buf;
}

Node *comb_node(double delay_time, double max_delay_time, double fb, double ff,
                Node *input) {

  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(comb_state));

  node->state_size = sizeof(comb_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  comb_state *state =
      (comb_state *)(graph->nodes_state_memory + node->state_offset);

  state->fb = fb; // feedback gain
  state->ff = ff; // feedforward gain
  state->write_pos = 0;

  // Calculate read position based on per-channel buffer size
  int delay_per_channel = bufsize / in_layout;
  state->read_pos = delay_per_channel - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)comb_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * BUF_SIZE)},
      .meta = "comb",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);

  return graph_embed(node);
}

// ---------------------- Lag Filter ---------------------------

typedef struct {
  sample_t current_value;
  sample_t target_value;
  sample_t coeff;
  sample_t lag_time;
} lag_state;

// Node *static_lag_node(sample_t lag_time, Node *input) {
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
//   sample_t spf = 1.0 / ctx_sample_rate();
//   state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));
//
//   // Connect input
//   node->connections[0].source_node_index = input->node_index;
//
//   return graph_embed(node);
// }
void *lag_perform(Node *node, lag_state *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *lag_time = inputs[1]->output.buf;

  while (nframes--) {
    sample_t lt = *lag_time;
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

  return graph_embed(node);
}

// ---------------------- Tanh Distortion ---------------------------

typedef struct {
  sample_t gain;
} tanh_state;

void *tanh_perform(Node *node, tanh_state *state, Node *inputs[], int nframes,
                   sample_t spf) {
  sample_t *out = node->output.buf;
  int in_layout = inputs[0]->output.layout;
  sample_t *in = inputs[0]->output.buf;

  sample_t gain = state->gain;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      *out = tanh(*in * gain);
      in++;
      out++;
    }
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
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, BUF_SIZE * input->output.layout)},
      .meta = "tanh",
  };

  // Initialize state
  tanh_state *state = state_ptr(graph, node);
  state->gain = gain;

  // Connect input
  plug_input_in_graph(0, node, input);

  return graph_embed(node);
}

// Node *tanh_node(sample_t gain, Node *input) {
//   Node *t = _tanh_node(gain, input);
//
//   return t;
// }

void *dyn_tanh_perform(Node *node, tanh_state *state, Node *inputs[],
                       int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  sample_t *gain = inputs[1]->output.buf;

  while (nframes--) {

    *out = tanhf(*in * *gain);
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
  return graph_embed(node);
}
typedef struct {
  int read_pos;
  int write_pos;
  sample_t g; // allpass gain coefficient
} allpass_state;

void *allpass_perform(Node *node, allpass_state *state, Node *inputs[],
                      int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *buf = inputs[1]->output.buf;
  int bufsize = inputs[1]->output.size;

  // Calculate delay buffer size per channel
  int delay_per_channel = bufsize / in_layout;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      // Calculate buffer offset for this channel
      int channel_offset = ch * delay_per_channel;

      // Get write and read pointers for this channel
      sample_t *write_ptr = buf + channel_offset + state->write_pos;
      sample_t *read_ptr = buf + channel_offset + state->read_pos;

      // Allpass filter equation: y[n] = -g * x[n] + x[n-M] + g * y[n-M]
      // where read_ptr contains the delayed input x[n-M] + g * y[n-M] from
      // previous iterations
      sample_t delayed_signal = *read_ptr;

      // Calculate output
      *out = -state->g * (*in) + delayed_signal;

      // Write to delay buffer: x[n] + g * y[n]
      *write_ptr = *in + state->g * (*out);

      in++;
      out++;
    }

    // Update positions once per frame (after processing all channels)
    state->read_pos = (state->read_pos + 1) % delay_per_channel;
    state->write_pos = (state->write_pos + 1) % delay_per_channel;
  }

  return node->output.buf;
}

Node *allpass_node(double delay_time, double max_delay_time, double g,
                   Node *input) {
  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(allpass_state));

  node->state_size = sizeof(allpass_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  allpass_state *state =
      (allpass_state *)(graph->nodes_state_memory + node->state_offset);

  state->g = g; // allpass gain coefficient (typically 0.0 to 0.99)
  state->write_pos = 0;

  // Calculate read position based on per-channel buffer size
  int delay_per_channel = bufsize / in_layout;
  state->read_pos = delay_per_channel - (int)(delay_time * sample_rate);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)allpass_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * BUF_SIZE)},
      .meta = "allpass",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);

  return graph_embed(node);
}

// ---------------------- Dynamic Allpass Filter ---------------------------

void *dyn_allpass_perform(Node *node, allpass_state *state, Node *inputs[],
                          int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  sample_t *delay_buf = inputs[1]->output.buf;
  int buf_size = inputs[1]->output.size;
  sample_t *delay_time = inputs[2]->output.buf;
  sample_t sample_rate = 1.0 / spf;

  // Calculate delay buffer size per channel
  int delay_per_channel = buf_size / in_layout;

  while (nframes--) {
    for (int ch = 0; ch < in_layout; ch++) {
      // Calculate buffer offset for this channel
      int channel_offset = ch * delay_per_channel;

      int write_pos = state->write_pos;
      int write_pos_abs = channel_offset + write_pos;

      // Calculate delay in samples and interpolation
      sample_t delay_samples = *delay_time * sample_rate;
      int read_offset = (int)delay_samples;
      sample_t frac = delay_samples - read_offset;

      // Calculate read positions with proper modulo
      int read_pos =
          (write_pos - read_offset + delay_per_channel) % delay_per_channel;
      int read_pos_next = (read_pos + 1) % delay_per_channel;

      // Add channel offset to get absolute positions
      int read_pos_abs = channel_offset + read_pos;
      int read_pos_next_abs = channel_offset + read_pos_next;

      // Linear interpolation for delayed signal
      sample_t delayed_signal = delay_buf[read_pos_abs] * (1.0 - frac) +
                                delay_buf[read_pos_next_abs] * frac;

      // Allpass filter equation: y[n] = -g * x[n] + x[n-M] + g * y[n-M]
      *out = -state->g * (*in) + delayed_signal;

      // Write to delay buffer: x[n] + g * y[n]
      delay_buf[write_pos_abs] = *in + state->g * (*out);

      in++;
      out++;
    }

    // Update positions once per frame
    state->write_pos = (state->write_pos + 1) % delay_per_channel;

    delay_time++;
  }

  return node->output.buf;
}

Node *dyn_allpass_node(double max_delay_time, double g, Node *input,
                       Node *delay_time) {
  int in_layout = input->output.layout;
  int sample_rate = ctx_sample_rate();
  int bufsize = (int)(max_delay_time * sample_rate * in_layout);
  Node *delay_buf = const_buf(0.0, 1, bufsize);

  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, sizeof(allpass_state));

  node->state_size = sizeof(allpass_state);
  node->state_offset = state_offset_ptr_in_graph(graph, node->state_size);

  allpass_state *state =
      (allpass_state *)(graph->nodes_state_memory + node->state_offset);

  state->g = g;
  state->write_pos = 0;
  state->read_pos = 0; // Dynamic read position

  *node = (Node){
      .perform = (perform_func_t)dyn_allpass_perform,
      .node_index = node->node_index,
      .num_inputs = 3,
      .state_size = node->state_size,
      .state_offset = node->state_offset,
      .output = (Signal){.layout = input->output.layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(
                             graph, input->output.layout * BUF_SIZE)},
      .meta = "dyn_allpass",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, delay_buf);
  plug_input_in_graph(2, node, delay_time);

  return graph_embed(node);
}
typedef struct grain_pitchshift_state {
  int length;
  int pos;
  int max_grains;
  int active_grains;
  int next_trig;
  int trig_gap_in_frames;
  sample_t width;
  sample_t rate;
  sample_t fb;
} grain_pitchshift_state;

sample_t __pow2table_read(sample_t pos, int tabsize, sample_t *table) {
  int mask = tabsize - 1;

  sample_t env_pos = pos * (mask);
  int env_idx = (int)env_pos;
  sample_t env_frac = env_pos - env_idx;

  // Interpolate between envelope table values
  sample_t env_val = table[env_idx & mask] * (1.0 - env_frac) +
                     table[(env_idx + 1) & mask] * env_frac;
  return env_val;
}

void *granular_pitchshift_perform(Node *node, grain_pitchshift_state *state,
                                  Node *inputs[], int nframes, sample_t spf) {

  int out_layout = node->output.layout;
  sample_t *out = node->output.buf;
  sample_t *in = inputs[0]->output.buf;
  char *mem = state + 1;
  sample_t *buf = mem;
  int buf_size = state->length;
  mem += buf_size * sizeof(sample_t);

  int max_grains = state->max_grains;

  sample_t *phases = (sample_t *)mem;
  mem += sizeof(sample_t) * max_grains;

  sample_t *widths = (sample_t *)mem;
  mem += sizeof(sample_t) * max_grains;

  sample_t *remaining_secs = (sample_t *)mem;
  mem += sizeof(sample_t) * max_grains;

  sample_t *starts = (sample_t *)mem;
  mem += sizeof(sample_t) * max_grains;

  int *active = (int *)mem;

  sample_t d_index;
  int index;
  sample_t frac;
  sample_t a, b;
  sample_t sample = 0.;
  sample_t r = state->rate;
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
        sample_t p = phases[i];
        sample_t s = starts[i];
        sample_t w = widths[i];
        sample_t rem = remaining_secs[i];

        d_index = s + (p * buf_size);

        index = (int)d_index;
        frac = d_index - index;

        a = buf[index % buf_size];
        b = buf[(index + 1) % buf_size];

        sample_t grain_elapsed = 1.0 - (rem / w);
        sample_t env_val =
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
                   (1024 * sizeof(sample_t))         // delay buffer
                   + (max_grains * sizeof(sample_t)) // phases
                   + (max_grains * sizeof(sample_t)) // widths
                   + (max_grains * sizeof(sample_t)) // elapsed
                   + (max_grains * sizeof(sample_t)) // starts
                   + (max_grains * sizeof(int))      // active grains
      ;

  grain_pitchshift_state pshift = {

      .length = 1024,
      .pos = 0,
      .max_grains = max_grains,
      .active_grains = 0,
      .next_trig = ((sample_t)1024) / shift,
      .trig_gap_in_frames = ((sample_t)1024) / shift,
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
  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  grain_pitchshift_state *state = mem;
  *state = pshift;

  plug_input_in_graph(0, node, input);

  return graph_embed(node);
}

typedef struct math_node_state {
  MathNodeFn math_fn;
} math_node_state;

void *math_perform(Node *node, math_node_state *state, Node *inputs[],
                   int nframes, sample_t spf) {
  sample_t *out = node->output.buf;
  Signal in = inputs[0]->output;

  for (int i = 0; i < in.size * in.layout; i++) {
    out[i] = (sample_t)state->math_fn((double)in.buf[i]);
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
  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  math_node_state *state = mem;
  *state = m;

  // node->connections[0].source_node_index = input->node_index;
  plug_input_in_graph(0, node, input);
  //
  return graph_embed(node);
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
                      int nframes, sample_t spf) {
  sample_t *buf_memory = (sample_t *)((stutter_state *)state + 1);

  Signal in = inputs[0]->output;
  sample_t *_gate = inputs[1]->output.buf;
  sample_t *_repeat_time = inputs[2]->output.buf;
  sample_t *out = node->output.buf;

  int chans = state->buf_chans;
  int sample_rate = (int)(1.0 / spf);

  for (int i = 0; i < nframes; i++) {
    sample_t gate = *_gate;
    _gate++;
    sample_t repeat_time = *_repeat_time;
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
  state_size += required_buf_frames * sizeof(sample_t);

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
  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  stutter_state *state = (stutter_state *)mem;
  *state = s;

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, gate);
  plug_input_in_graph(2, node, repeat_time);
  return graph_embed(node);
}

typedef struct {
  void *opaque_ref;
} opaque_ref_node_state;

NodeRef wrap_opaque_ref_in_node(void *opaque_ref, void *perform, int out_chans,
                                NodeRef input) {

  AudioGraph *graph = _graph;

  int state_size = sizeof(opaque_ref_node_state);

  opaque_ref_node_state m = {.opaque_ref = opaque_ref};

  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = out_chans,
                         .size = input->output.size,
                         .buf = allocate_buffer_from_pool(
                             graph, out_chans * input->output.size)},
      .meta = "opaque_ref_filter_node",
  };

  /* Initialize state memory */
  char *mem = state_ptr(graph, node);

  memset(mem, 0, state_size);
  opaque_ref_node_state *state = (opaque_ref_node_state *)mem;
  *state = m;

  // node->connections[0].source_node_index = input->node_index;
  plug_input_in_graph(0, node, input);
  //
  return graph_embed(node);
}
