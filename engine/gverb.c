#include "./primes.h"
#include "audio_graph.h"
#include "ctx.h"
#include "node.h"
#include <stdio.h>
#include <string.h>

#define MAX_DELAY_LENGTH 20000
#define NUM_EARLY_REFLECTIONS 8
#define NUM_ALLPASS 4

double __early_gain_factors[NUM_EARLY_REFLECTIONS] = {1.0f, 0.9f, 0.8f, 0.7f,
                                                      0.6f, 0.5f, 0.4f, 0.3f};
typedef struct {
  double room_size;       /* Room size in meters */
  double reverb_time;     /* RT60 reverb time in seconds */
  double damping;         /* Damping factor (0-1) */
  double input_bandwidth; /* Input bandwidth (0-1) */
  double dry;             /* Dry signal level (0-1) */
  double wet;             /* Wet signal level (0-1) */
  double early_level;     /* Early reflections level (0-1) */
  double tail_level;      /* Tail level (0-1) */

  double sample_rate;

  // double *early_delays[NUM_EARLY_REFLECTIONS]; // allocated outside the
  // struct
  int early_lengths[NUM_EARLY_REFLECTIONS];
  int early_pos[NUM_EARLY_REFLECTIONS];
  double early_gains[NUM_EARLY_REFLECTIONS];

  // double *delay_lines[4]; /* Main delay lines -- allocated outside the struct
  // */
  int delay_lengths[4]; /* Length of each delay line */
  int delay_pos[4];     /* Current position in each delay line */

  // double *allpass_delays[NUM_ALLPASS]; allocated outside the struct
  int allpass_lengths[NUM_ALLPASS];
  int allpass_pos[NUM_ALLPASS];
  double allpass_feedback;

  double damping_coeff;
  double damping_state[4];

  double input_filter_state;
  double input_filter_coeff;

} GVerb;

/*
 * Helper functions
 */

/* Next prime number after n */
static int next_prime(int n) {
  int i = 0;
  while (PRIMES[i] <= n && i < NUM_PRIMES) {
    i++;
  }
  return PRIMES[i];
}

/* Generate a delay length based on room size in meters */
static int compute_delay_length(double room_size, double sample_rate,
                                double scaling_factor) {
  /* Speed of sound = 343 m/s at 20°C */
  double delay_sec = (room_size * scaling_factor) / 343.0f;
  int delay_samples = (int)(delay_sec * sample_rate);

  return next_prime(delay_samples);
}

static double lowpass_filter(double input, double *state, double coeff) {
  double output = input * (1.0f - coeff) + *state * coeff;
  *state = output;
  return output;
}

static void delay_line_write(double *delay_line, int pos, int length,
                             double value) {
  delay_line[pos] = value;
}

static double delay_line_read(double *delay_line, double pos_double,
                              int length) {
  int pos_int = (int)pos_double;
  double frac = pos_double - pos_int;

  int pos1 = pos_int;
  int pos2 = (pos_int + 1) % length;

  return delay_line[pos1] * (1.0f - frac) + delay_line[pos2] * frac;
}

static double allpass_process(double input, double *delay_line, int *pos,
                              int length, double feedback) {
  double delayed = delay_line[*pos];

  double new_input = input + feedback * delayed;

  delay_line[*pos] = new_input;

  *pos = (*pos + 1) % length;

  return delayed - feedback * new_input;
}

double process_early_reflection(double in, int *write_pos, int delay_samples,
                                double *buffer, double gain) {
  buffer[*write_pos] = in;
  int read_pos = (*write_pos - delay_samples) % delay_samples;
  *write_pos = (*write_pos + 1) % delay_samples;
  return buffer[read_pos] * gain;
}

double __gverb_process(GVerb *gverb, double in_sample, double **early_delays,
                       double **delay_lines, double **allpass_delays) {
  int i;
  double out_sample = 0.0f;
  double early_out = 0.0f;
  double diffuse_out = 0.0f;

  double diffuse_signal = 0.0f;

  /* Apply input bandwidth filter (pre-filtering) */
  double filtered_input = lowpass_filter(in_sample, &gverb->input_filter_state,
                                         gverb->input_filter_coeff);

  /* Process early reflections - simplified position handling */
  for (i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    early_out += process_early_reflection(
        filtered_input, &gverb->early_pos[i], gverb->early_lengths[i],
        early_delays[i], __early_gain_factors[i]);

    // printf("early out %f %d %f\n", early_out, gverb->early_pos[i],
    //        gverb->early_gains[i]);
  }

  /*
  /* Process diffuse reverb tail using FDN */
  double input_mix = filtered_input * 0.5f + early_out * 0.5f;
  //
  // /* Read from delay lines at current positions */
  // double delay_out[4];
  // for (i = 0; i < 4; i++) {
  //   delay_out[i] = delay_lines[i][gverb->delay_pos[i]];
  // }
  //
  // /* Apply damping to delay line outputs */
  // for (i = 0; i < 4; i++) {
  //   delay_out[i] = lowpass_filter(delay_out[i], &gverb->damping_state[i],
  //                                 gverb->damping_coeff);
  // }
  //
  // /* Hadamard matrix multiplication for diffusion */
  // double feedback[4];
  // feedback[0] = delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3];
  // feedback[1] = delay_out[0] + delay_out[1] - delay_out[2] - delay_out[3];
  // feedback[2] = delay_out[0] - delay_out[1] + delay_out[2] - delay_out[3];
  // feedback[3] = delay_out[0] - delay_out[1] - delay_out[2] + delay_out[3];
  //
  // /* Apply reverb time scaling */
  // double rt_gain = powf(10.0f, -3.0f * (1.0f / gverb->reverb_time));
  // for (i = 0; i < 4; i++) {
  //   feedback[i] *= 0.25f * rt_gain; /* 0.25 for Hadamard normalization */
  // }
  //
  // /* Add the input to the first delay line */
  // feedback[0] += input_mix;
  //
  // /* Write to delay lines and advance positions */
  // for (i = 0; i < 4; i++) {
  //   delay_lines[i][gverb->delay_pos[i]] = feedback[i];
  //   gverb->delay_pos[i] = (gverb->delay_pos[i] + 1) %
  //   gverb->delay_lengths[i];
  // }
  //
  // /* Additional diffusion through all-pass filters */
  // double diffuse_signal =
  //     (delay_out[0] + delay_out[1] + delay_out[2] + delay_out[3]) * 0.25f;
  //
  // for (i = 0; i < NUM_ALLPASS; i++) {
  //   diffuse_signal = allpass_process(
  //       diffuse_signal, allpass_delays[i], &gverb->allpass_pos[i],
  //       gverb->allpass_lengths[i], gverb->allpass_feedback);
  // }

  /* Final output mixing */
  out_sample = in_sample * gverb->dry +
               early_out * gverb->early_level * gverb->wet +
               diffuse_signal * gverb->tail_level * gverb->wet;

  return out_sample;
}

double gverb_process(GVerb *gverb, double in_sample, double **early_delays,
                     double **delay_lines, double **allpass_delays) {
  int i;
  double out_sample = 0.0f;
  double early_out = 0.0f;
  double diffuse_out = 0.0f;

  double diffuse_signal = 0.0f;

  double filtered_input = lowpass_filter(in_sample, &gverb->input_filter_state,
                                         gverb->input_filter_coeff);

  for (i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    early_out += process_early_reflection(
        filtered_input, &gverb->early_pos[i], gverb->early_lengths[i],
        early_delays[i], __early_gain_factors[i]);
  }

  double input_mix = filtered_input * 0.5f + early_out * 0.5f;
  out_sample = in_sample * gverb->dry +
               early_out * gverb->early_level * gverb->wet +
               diffuse_signal * gverb->tail_level * gverb->wet;

  return out_sample;
}

void *gverb_perform(Node *node, GVerb *state, Node *inputs[], int nframes,
                    double spf) {

  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  char *mem = (char *)((GVerb *)state + 1);
  double *early_delays[NUM_EARLY_REFLECTIONS];

  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    early_delays[i] = (double *)mem;
    mem += sizeof(double) * state->early_lengths[i];
  }
  double *delay_lines[4];

  for (int i = 0; i < 4; i++) {
    delay_lines[i] = (double *)mem;
    mem += sizeof(double) * state->delay_lengths[i];
  }
  double *allpass_delays[NUM_ALLPASS];
  for (int i = 0; i < NUM_ALLPASS; i++) {
    allpass_delays[i] = (double *)mem;
    mem += sizeof(double) * state->allpass_lengths[i];
  }

  while (nframes--) {
    *out = gverb_process(state, *in, early_delays, delay_lines, allpass_delays);

    in++;
    out++;
  }
  return node->output.buf;
}

void gverb_set_reverb_time(GVerb *gverb, double reverb_time) {
  gverb->reverb_time = reverb_time;
  float mean_delay_time = 0.0f;
  int i;

  for (i = 0; i < 4; i++) {
    mean_delay_time += gverb->delay_lengths[i] / gverb->sample_rate;
  }
  mean_delay_time /= 4.0f;

  float max_reverb_time = 20.0f;
  if (reverb_time > max_reverb_time)
    reverb_time = max_reverb_time;

  float decay_per_second = -60.0f / reverb_time;
  float decay_per_sample = decay_per_second / gverb->sample_rate;

  for (i = 0; i < 4; i++) {
    float delay_time_sec = gverb->delay_lengths[i] / gverb->sample_rate;
    float decay_for_this_delay = decay_per_sample * gverb->delay_lengths[i];
    float linear_decay = powf(10.0f, decay_for_this_delay / 20.0f);

    /* Store this in the filter processing */
    /* The feedback coefficient is applied in the main processing function */
  }
}

void gverb_set_damping(GVerb *gverb, double damping) {
  gverb->damping = damping;
  gverb->damping_coeff = damping * 0.9f + 0.1f;
}

void gverb_set_input_bandwidth(GVerb *gverb, double input_bandwidth) {
  gverb->input_bandwidth = input_bandwidth;

  /* Set the input filter coefficient (lowpass) */
  gverb->input_filter_coeff = input_bandwidth;
}

// Or implement zero_gverb_filter_state:
void zero_gverb_filter_state(GVerb *state) {
  memset(&state->damping_state, 0, sizeof(state->damping_state));
  state->input_filter_state = 0.0;
}

Node *gverb_node(Node *input) {
  double sr = (double)ctx_sample_rate();

  GVerb gverb = {
      .room_size = 5.,
      .reverb_time = 10.,
      .damping = 0.2,
      .input_bandwidth = 0.5,
      .dry = 0.1,
      .wet = 1.,
      .early_level = 0.5,
      .tail_level = 0.5,
      .allpass_feedback = 0.6,
      .sample_rate = sr,
  };

  AudioGraph *graph = _graph;
  int state_size = sizeof(GVerb);

  int i;
  double early_scaling_factors[NUM_EARLY_REFLECTIONS] = {
      1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f};

  double early_gain_factors[NUM_EARLY_REFLECTIONS] = {1.0f, 0.9f, 0.8f, 0.7f,
                                                      0.6f, 0.5f, 0.4f, 0.3f};
  for (i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    gverb.early_lengths[i] =
        compute_delay_length(gverb.room_size, sr, early_scaling_factors[i]);

    state_size += gverb.early_lengths[i] * sizeof(double);

    gverb.early_pos[i] = 0;
    gverb.early_gains[i] = early_gain_factors[i];
  }

  double delay_scaling_factors[4] = {1.0f, 0.9f, 0.8f, 0.7f};
  for (i = 0; i < 4; i++) {
    gverb.delay_lengths[i] =
        compute_delay_length(gverb.room_size, sr, delay_scaling_factors[i]);

    state_size += gverb.delay_lengths[i] * sizeof(double);

    gverb.delay_pos[i] = 0;
  }

  double allpass_scaling_factors[NUM_ALLPASS] = {0.15f, 0.12f, 0.10f, 0.08f};
  for (i = 0; i < NUM_ALLPASS; i++) {
    gverb.allpass_lengths[i] =
        compute_delay_length(gverb.room_size, sr, allpass_scaling_factors[i]);
    state_size += gverb.allpass_lengths[i] * sizeof(double);
    gverb.allpass_pos[i] = 0;
  }

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

  GVerb *state = (GVerb *)(graph->nodes_state_memory + node->state_offset);
  memset((char *)state, 0, state_size);

  *state = gverb;

  /* Set up initial filter coefficients */
  gverb_set_reverb_time(state, state->reverb_time);
  gverb_set_damping(state, state->damping);
  gverb_set_input_bandwidth(state, state->input_bandwidth);
  zero_gverb_filter_state(state);

  node->connections[0].source_node_index = input->node_index;

  for (int i = 0; i < NUM_EARLY_REFLECTIONS; i++) {
    // state->early_lengths[i] = gverb.early_lengths[i];
    printf("early length %d: %d gain %f\n", i, state->early_lengths[i],
           state->early_gains[i]);
  }
  // for (int i = 0; i < 4; i++) {
  //   // state->delay_lengths[i] = gverb.delay_lengths[i];
  //   printf("delay length %d: %d\n", i, state->delay_lengths[i]);
  // }
  //
  // for (int i = 0; i < NUM_ALLPASS; i++) {
  //   // state->allpass_lengths[i] = gverb.allpass_lengths[i];
  //   printf("ap length %d: %d\n", i, state->allpass_lengths[i]);
  // }
  // state->early_gains = early_gain_factors;
  return node;
}
