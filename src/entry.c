#include "entry.h"
#include "common.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

double random_double() {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * 2 - 1;
  return rand_double;
}

double random_double_range(double min, double max) {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

node_perform noise_perform(Node *node, int nframes, double spf) {
  Signal *out = node->out;

  while (nframes--) {
    for (int ch = 0; ch < out->layout; ch++) {
      *out->data = random_double();
      out->data++;
    }
  }
}

// ----------------------------- SINE WAVE OSCILLATORS
//
#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);

    // printf("%f\n", val);
    sin_table[i] = val;
    phase += phsinc;
  }
}

node_perform sine_perform(Node *node, int nframes, double spf) {
  sin_data *data = node->data;
  double *out = node->out->data;
  double *input = node->ins->data;

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *input;

    d_index = data->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    *out = sample;
    data->phase = fmod(freq * spf + (data->phase), 1.0);

    out++;
    input++;
  }
}

Node *sine(double freq) {
  sin_data *dsq = malloc(sizeof(sin_data));
  dsq->phase = 0.0;

  Signal *in = get_sig(1);
  for (int i = 0; i < in->size; i++) {
    in->data[i] = freq;
  }

  Node *s = node_new(dsq, (node_perform *)sine_perform, in, get_sig(1));
  s->ins = in;
  return s;
}

// ----------------------------- SQUARE WAVE OSCILLATORS
//
double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

node_perform sq_perform(Node *node, int nframes, double spf) {
  sq_data *data = node->data;

  double *out = node->out->data;
  double *in = node->ins->data;

  while (nframes--) {
    double samp =
        sq_sample(data->phase, *in) + sq_sample(data->phase, *in * 1.01);
    samp /= 2;

    data->phase += spf;
    *out = samp;
    out++;
    in++;
  }
}

Node *sq_node(double freq) {
  sq_data *dsq = malloc(sizeof(sq_data));
  dsq->phase = 0.0;
  Signal *in = get_sig(1);
  int n = in->size;
  for (int i = 0; i < n; i++) {
    in->data[i] = freq;
  }

  Node *s = node_new(dsq, sq_perform, in, get_sig(1));
  s->ins = in;
  return s;
}

node_perform lf_noise_perform(Node *node, int nframes, double spf) {
  lf_noise_data *data = node->data;

  double noise_freq = data->freq;
  int trigger_per_ms = (int)(1000 / noise_freq);
  double *out = node->out->data;
  // Signal in = node->ins;

  while (nframes--) {

    if ((int)(1000 * data->phase) >= trigger_per_ms) {
      data->phase = 0;
      data->target = random_double_range(data->min, data->max);
      // printf("noise target: %f [%f, %f]\n", data->target, data->min,
      // data->max);
    }

    data->phase += spf;
    *out = data->target;
    out++;
    // in.data++;
  }
}

// ------------------------------------------- DISTORTION
typedef struct {
  double gain;
} tanh_data;

node_perform tanh_perform(Node *node, int nframes, double spf) {
  tanh_data *data = node->data;
  double *in = node->ins->data;
  double *out = node->out->data;
  while (nframes--) {
    *out = tanh(data->gain * (*in));
    in++;
    out++;
  }
}

Node *tanh_node(double gain, Node *node) {
  tanh_data *data = malloc(sizeof(tanh_data));
  data->gain = gain;
  Signal *in = node->out;
  Signal *out = node->out;

  Node *s = node_new(data, tanh_perform, in, out);
  return s;
}

// ------------------------------------------- NOISE UGENS

Node *lfnoise(double freq, double min, double max) {
  lf_noise_data *noise_d = malloc(sizeof(lf_noise_data));
  noise_d->phase = 0.0;
  noise_d->target = random_double_range(min, max);
  noise_d->min = min;
  noise_d->max = max;
  noise_d->freq = freq;

  return node_new(noise_d, lf_noise_perform, &(Signal){}, get_sig(1));
}

node_perform lf_noise_interp_perform(Node *node, int nframes, double spf) {
  double noise_freq = 5.0;
  int trigger_per_ms = (int)(1000 / noise_freq);

  lf_noise_interp_data *data = node->data;

  // printf("noise: %f %f\n", data->val, data->phase);

  double *out = node->out->data;

  while (nframes--) {

    // printf("noise: %f %d thresh: %d\n", data->target, (int)data->phase,
    // trigger_per_ms);
    if ((int)(1000 * data->phase) >= trigger_per_ms) {
      data->phase = 0;
      data->target = random_double_range(60, 120);
      // printf("noise: %f\n", data->target);
    }

    data->phase += spf;
    *out = data->current;
    data->current = data->current + 0.000001 * (data->target - data->current);
    out++;
  }
}

// ------------------------------ SIGNAL / BUFFER ALLOC
//
static double buf_pool[BUF_SIZE * LAYOUT_CHANNELS * 100];
static double *buf_ptr = buf_pool;

void init_sig_ptrs() { buf_ptr = buf_pool; }

Signal *get_sig(int layout) {
  Signal *sig = malloc(sizeof(Signal));
  sig->data = buf_ptr;
  sig->layout = layout;
  sig->size = BUF_SIZE;
  buf_ptr += BUF_SIZE * layout;
  return sig;
}
// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out) {
  Node *node = malloc(sizeof(Node));
  node->data = data;
  node->ins = ins;
  node->out = out;
  node->perform = (node_perform)perform;
  return node;
}

Node *pipe_output(Node *send, Node *recv) {
  recv->ins = send->out;
  return recv;
}
Node *add_to_dac(Node *node) {
  node->type = OUTPUT;
  return node;
}

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, &(Signal){}); }

Node *add_to_chain(Node *chain, Node *node) {
  if (chain->head == NULL) {
    chain->head = node;
    chain->tail = node;
    return node;
  }

  chain->tail->next = node;
  chain->tail = node;
  return node;
}

int entry() {
  Ctx *ctx = get_audio_ctx();
  Node *chain = chain_new();
  Node *noise = add_to_chain(chain, lfnoise(8., 80., 500.));
  Node *nsq = add_to_chain(chain, sine(100.0));
  pipe_output(noise, nsq);

  add_to_dac(chain);
  ctx_add(chain);
  sleep(10);
}
