#include "oscillators.h"
#include "common.h"
#include "ctx.h"
#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct sq_state {
  double phase;
} sq_state;
#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {0};

// void maketable_sq(void) {
//   double phase = 0.0;
//   double phsinc = (2. * PI) / SIN_TABSIZE;
//
//   for (int i = 0; i < SIN_TABSIZE; i++) {
//     // for (int harm = 1; harm < 5; harm++) {
//     // }
//     double val = sin(phase);
//
//     sin_table[i] += val;
//     printf("%f\n", sin_table[i]);
//
//     phase += phsinc;
//   }
// }
void maketable_sq(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SQ_TABSIZE;
  for (int i = 0; i < SQ_TABSIZE; i++) {

    for (int harm = 1; harm < SQ_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      // for (int harm = 1; harm < 100; harm += 2) { // summing odd frequencies
      double val = sin(phase * harm) / harm; // sinewave of different frequency
      sq_table[i] += val;
    }
    phase += phsinc;
  }
}

double sq_sample(double phase) {
  phase = fmod(phase, 1.0);

  double d_index;
  int index;
  double frac, a, b;
  double sample;

  d_index = phase * SQ_TABSIZE;
  index = (int)d_index;
  frac = d_index - index;

  a = sq_table[index];
  b = sq_table[(index + 1) % SQ_TABSIZE];

  sample = (1.0 - frac) * a + (frac * b);
  return sample;
  // *out = sample;
}

node_perform sq_perform(Node *node, int nframes, double spf) {

  sq_state *state = (sq_state *)node->state;

  double *input = node->ins[0].buf;
  double *out = node->out.buf;

  while (nframes--) {
    *out = sq_sample(state->phase);
    // *out += sq_sample(state->phase * 1.5, *input);
    // *out /= 2.;
    // printf("sq perform *out %f\n", *out);
    state->phase = fmod(*input * spf + (state->phase), 1.0);
    out++;
    input++;
  }
}

Node *sq_node(Signal *freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sq_perform, 1, freq);

  return s;
}
typedef struct sin_state {
  double phase;
} sin_state;

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

node_perform sin_perform(Node *node, int nframes, double spf) {

  sin_state *state = (sin_state *)node->state;
  double *input = node->ins[0].buf;
  double *out = node->out.buf;

  // printf("read freq input %p\n", input);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *input;
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    *out = sample;
    state->phase = fmod(freq * spf + (state->phase), 1.0);

    out++;
    input++;
  }
}

Node *sin_node(Signal *freq) {
  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sin_perform, 1, freq);
  return s;
}

typedef struct bufplayer_state {
  double phase;
} bufplayer_state;

node_perform _bufplayer_perform(Node *node, int nframes, double spf) {

  bufplayer_state *state = (bufplayer_state *)node->state;
  double *buf = node->ins[0].buf;
  int buf_size = node->ins[0].size;

  double *rate = node->ins[1].buf;

  double *out = node->out.buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    d_index = state->phase * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);

    state->phase = fmod(state->phase + (*rate) / buf_size, 1.0);

    *out = sample;

    out++;
    rate++;
  }
}

Node *_bufplayer_node(Signal *buf, Signal *rate) {
  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;

  Signal *sigs = malloc(sizeof(Signal) * 2);
  sigs[0].buf = buf->buf;
  sigs[0].size = buf->size;
  sigs[0].layout = buf->layout;

  sigs[1].buf = rate->buf;
  sigs[1].size = rate->size;
  sigs[1].layout = rate->layout;

  Node *_bufplayer =
      node_new(state, (node_perform *)_bufplayer_perform, 2, sigs);

  // printf("create _bufplayer node %p with buf signal %p\n", _bufplayer, buf);
  return _bufplayer;
}

node_perform _bufplayer_trig_perform(Node *node, int nframes, double spf) {

  bufplayer_state *state = (bufplayer_state *)node->state;
  double *buf = node->ins[0].buf;
  int buf_size = node->ins[0].size;

  double *rate = node->ins[1].buf;

  double *out = node->out.buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    d_index = state->phase * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);

    state->phase = fmod(state->phase + (*rate) / buf_size, 1.0);

    *out = sample;

    out++;
    rate++;
  }
}

node_perform bufplayer_trig_perform(Node *node, int nframes, double spf) {
  bufplayer_state *state = node->state;
  int chans = node->ins[0].layout;
  double *buf = node->ins[0].buf;
  double *rate = node->ins[1].buf;
  double *trig = node->ins[2].buf;
  double *start_pos = node->ins[3].buf;

  int buf_size = node->ins[0].size;
  double *out = node->out.buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    if (*trig == 1.0) {
      state->phase = 0;
    }

    d_index = (fmod(state->phase + *start_pos, 1.0)) * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);
    state->phase = fmod(state->phase + *rate / buf_size, 1.0);
    *out = sample;

    out++;
    rate++;
    trig++;
    start_pos++;
  }
}

node_perform bufplayer_1shot_trig_perform(Node *node, int nframes, double spf) {
  bufplayer_state *state = node->state;
  int chans = node->ins[0].layout;
  double *buf = node->ins[0].buf;
  double *rate = node->ins[1].buf;
  double *trig = node->ins[2].buf;
  double *start_pos = node->ins[3].buf;

  int buf_size = node->ins[0].size;
  double *out = node->out.buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    if (*trig == 1.0) {
      state->phase = 0;
    }

    d_index = (fmod(state->phase + *start_pos, 1.0)) * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);
    double pos = state->phase + *rate / buf_size;
    state->phase = pos > 1.0 ? 1.0 : pos;
    *out = sample;

    out++;
    rate++;
    trig++;
    start_pos++;
  }
}

Node *bufplayer_node(Signal *buf, Signal *rate, Signal *start_pos,
                     Signal *trig) {
  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;

  Signal *sigs = malloc(sizeof(Signal) * 4);
  sigs[0].buf = buf->buf;
  sigs[0].size = buf->size;
  sigs[0].layout = buf->layout;

  sigs[1].buf = rate->buf;
  sigs[1].size = rate->size;
  sigs[1].layout = rate->layout;

  // sigs[2].buf = calloc(BUF_SIZE, sizeof(double));
  // // sigs[2].buf[0] = 1.0;
  // sigs[2].size = BUF_SIZE;
  // sigs[2].layout = 1;

  sigs[2].buf = trig->buf;
  sigs[2].size = trig->size;
  sigs[2].layout = trig->layout;

  sigs[3].buf = start_pos->buf;
  sigs[3].size = start_pos->size;
  sigs[3].layout = start_pos->layout;

  Node *_bufplayer =
      node_new(state, (node_perform *)bufplayer_trig_perform, 4, sigs);

  // printf("create _bufplayer node %p with buf signal %p\n", _bufplayer, buf);
  return _bufplayer;
}

Node *bufplayer_1shot_node(Signal *buf, Signal *rate, Signal *start_pos,
                           Signal *trig) {
  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;

  Signal *sigs = malloc(sizeof(Signal) * 4);
  sigs[0].buf = buf->buf;
  sigs[0].size = buf->size;
  sigs[0].layout = buf->layout;

  sigs[1].size = rate->size;
  sigs[1].layout = rate->layout;
  sigs[1].buf = rate->buf;

  // sigs[2].buf = calloc(BUF_SIZE, sizeof(double));
  // // sigs[2].buf[0] = 1.0;
  // sigs[2].size = BUF_SIZE;
  // sigs[2].layout = 1;

  sigs[2].buf = trig->buf;
  for (int i = 0; i < trig->size; i++) {
    sigs[2].buf[i] = 0.0;
  }
  sigs[2].size = trig->size;
  sigs[2].layout = trig->layout;

  sigs[3].buf = start_pos->buf;
  sigs[3].size = start_pos->size;
  sigs[3].layout = start_pos->layout;

  Node *_bufplayer =
      node_new(state, (node_perform *)bufplayer_1shot_trig_perform, 4, sigs);

  // printf("create _bufplayer node %p with buf signal %p\n", _bufplayer, buf);
  return _bufplayer;
}

int _rand_int(int range) {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * range;
  return (int)rand_double;
}
double _random_double() {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * 2 - 1;
  return rand_double;
}

double _random_double_range(double min, double max) {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

node_perform white_noise_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  while (nframes--) {
    *out = _random_double_range(-1.0, 1.0);
    out++;
  }
}

Node *white_noise_node() {

  Node *noise = node_new(NULL, (node_perform *)white_noise_perform, 0, NULL);
  return noise;
}

typedef struct brown_noise_state {
  double last;
} brown_noise_state;

node_perform brown_noise_perform(Node *node, int nframes, double spf) {
  double scale = sqrt(spf);
  // double scale = spf;
  brown_noise_state *st = node->state;
  double *out = node->out.buf;

  while (nframes--) {
    double add = scale * _random_double_range(-1.0, 1.0);
    st->last += add;
    // Prevent unbounded drift
    if (st->last > 1.0 || st->last < -1.0) {
      st->last = st->last * 0.999;
    }
    *out = st->last;
    out++;
  }
}

Node *brown_noise_node() {
  brown_noise_state *st = malloc(sizeof(brown_noise_state));
  Node *noise = node_new(st, (node_perform *)brown_noise_perform, 0, NULL);
  return noise;
}

Node *lfnoise_node() {}

typedef struct chirp_state {
  double current_freq;
  double target_freq;
  double coeff;
  int trigger_active;
  double start_freq;
  double end_freq;
  // double lag_time;
  double elapsed_time;
} chirp_state;

node_perform chirp_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  double *trig = node->ins[0].buf;
  double *lag = node->ins[1].buf;

  chirp_state *state = (chirp_state *)node->state;

  double lag_time = *lag;
  while (nframes--) {
    lag_time = *lag;
    if (*trig == 1.0) {
      state->trigger_active = 1;
      state->current_freq = state->start_freq;
      state->elapsed_time = 0.0;
    }

    if (state->trigger_active) {
      // Calculate progress (0 to 1)
      double progress = state->elapsed_time / lag_time;
      if (progress > 1.0)
        progress = 1.0;

      // Use exponential interpolation for frequency
      state->current_freq = state->start_freq *
                            pow(state->end_freq / state->start_freq, progress);

      // Output the current frequency
      *out = state->current_freq;

      // Update elapsed time
      state->elapsed_time += spf;

      // Check if we've reached the end of the chirp
      if (state->elapsed_time >= lag_time) {
        state->trigger_active = 0;
        state->current_freq = state->end_freq;
      }
    } else {
      *out = state->current_freq; // Output last frequency when not active
    }
    lag++;
    out++;
    trig++;
  }
}

Node *chirp_node(double start, double end, Signal *lag_time, Signal *trig) {
  chirp_state *state = malloc(sizeof(chirp_state));
  state->current_freq = start;
  state->target_freq = end;
  state->trigger_active = 0;
  state->start_freq = start;
  state->end_freq = end;
  state->elapsed_time = 0.0;

  Signal *sigs = malloc(sizeof(Signal) * 2);

  sigs[0].buf = trig->buf;
  sigs[0].size = trig->size;
  sigs[0].layout = trig->layout;

  sigs[1].buf = lag_time->buf;
  sigs[1].size = lag_time->size;
  sigs[1].layout = lag_time->layout;

  Node *chirp = node_new(state, (node_perform *)chirp_perform, 2, sigs);

  return chirp;
}

typedef struct nbl_impulse_state {
  double phase;
} nbl_impulse_state;

node_perform nbl_impulse_perform(Node *node, int nframes, double spf) {
  nbl_impulse_state *state = (nbl_impulse_state *)node->state;
  double *input = node->ins[0].buf;
  double *out = node->out.buf;
  double sample;
  double freq;

  while (nframes--) {
    freq = *input;

    if (state->phase == 1.0) {
      sample = 1.0;
    } else {
      sample = 0.0;
    }
    *out = sample;
    state->phase = state->phase - (freq * spf);
    if (state->phase <= 0.0) {
      state->phase = 1.0;
    }

    out++;
    input++;
  }
}

Node *nbl_impulse_node(Signal *freq) {
  nbl_impulse_state *state = malloc(sizeof(nbl_impulse_state));
  state->phase = 1.0;
  Signal *sigs = malloc(sizeof(Signal) * 1);
  sigs[0].buf = freq->buf;
  sigs[0].size = freq->size;
  sigs[0].layout = freq->layout;
  Node *nbl_impulse =
      node_new(state, (node_perform *)nbl_impulse_perform, 1, sigs);
  return nbl_impulse;
}

typedef struct ramp_state {
  double phase;
} ramp_state;

node_perform ramp_perform(Node *node, int nframes, double spf) {
  ramp_state *state = (ramp_state *)node->state;
  double *input = node->ins[0].buf;
  double *out = node->out.buf;
  double sample;
  double freq;

  while (nframes--) {
    freq = *input;

    *out = state->phase;
    state->phase = state->phase + (freq * spf);
    if (state->phase >= 1.0) {
      state->phase = 0.0;
    }

    out++;
    input++;
  }
}

Node *ramp_node(Signal *freq) {
  ramp_state *state = malloc(sizeof(ramp_state));
  state->phase = 0.0;
  Signal *sigs = malloc(sizeof(Signal) * 1);
  sigs[0].buf = freq->buf;
  sigs[0].size = freq->size;
  sigs[0].layout = freq->layout;
  Node *ramp = node_new(state, (node_perform *)ramp_perform, 1, sigs);
  return ramp;
}

node_perform trig_rand_perform(Node *node, int nframes, double spf) {
  double *trig = node->ins[0].buf;
  double *out = node->out.buf;
  double prev = *out;

  while (nframes--) {
    if (*trig == 1.0) {
      *out = _random_double_range(0.0, 1.0);
      prev = *out;
    } else {
      *out = prev;
    }
    out++;
    trig++;
  }
}

Node *trig_rand_node(Signal *trig) {
  Signal *sigs = malloc(sizeof(Signal) * 1);
  sigs[0].buf = trig->buf;
  sigs[0].size = trig->size;
  sigs[0].layout = trig->layout;
  Node *trig_rand = node_new(NULL, (node_perform *)trig_rand_perform, 1, sigs);
  double r = _random_double_range(0.0, 1.0);
  for (int i = 0; i < trig_rand->out.size; i++) {
    trig_rand->out.buf[i] = r;
  }
  return trig_rand;
}

node_perform trig_sel_perform(Node *node, int nframes, double spf) {
  double *trig = node->ins[0].buf;
  double *sels = node->ins[1].buf;
  int sels_size = node->ins[1].size;
  double *out = node->out.buf;
  double prev = *out;

  while (nframes--) {
    if (*trig == 1.0) {
      *out = sels[rand() % sels_size];
      prev = *out;
    } else {
      *out = prev;
    }
    out++;
    trig++;
  }
}

Node *trig_sel_node(Signal *trig, Signal *sels) {
  Signal *sigs = malloc(sizeof(Signal) * 2);
  sigs[0].buf = trig->buf;
  sigs[0].size = trig->size;
  sigs[0].layout = trig->layout;

  sigs[1].buf = sels->buf;
  sigs[1].size = sels->size;
  sigs[1].layout = sels->layout;

  Node *trig_rand = node_new(NULL, (node_perform *)trig_sel_perform, 2, sigs);
  double r = sigs[1].buf[rand() % sigs[1].size];
  for (int i = 0; i < trig_rand->out.size; i++) {
    trig_rand->out.buf[i] = r;
  }
  return trig_rand;
}

typedef struct {
  int start;
  int length;
  int position;
  double rate;
  double amp;
} Grain;

typedef struct granulator_state {
  int *starts;
  int *lengths;
  int *positions;
  double *rates;
  double *amps;

  int max_concurrent_grains;
  int active_grains;
  int min_grain_length;
  int max_grain_length;
  double overlap;
  int next_free_grain; // field to keep track of the next available grain
} granulator_state;

static double rates[] = {1.0, 1.5, 2.0, 0.5, 0.75};

static inline void init_grain(granulator_state *state, int index, double pos,
                              int length, double rate) {

  state->starts[index] = (int)pos;
  state->lengths[index] = length;
  state->positions[index] = 0.0;
  state->rates[index] = rate;
  state->active_grains++;
}
static inline void process_grain(granulator_state *state, int i, double *out,
                                 double *buf, int buf_size) {
  // Linear interpolation
  double d_index = state->starts[i] + state->positions[i] * state->rates[i];

  int index = (int)d_index;
  double frac = d_index - index;

  double a = buf[index % buf_size];
  double b = buf[(index + 1) % buf_size];

  double sample = ((1.0 - frac) * a + (frac * b)) * state->amps[i];
  *out += sample;

  state->positions[i] += 1;
  if (state->positions[i] >= state->lengths[i]) {
    state->lengths[i] = 0;
    state->active_grains--;
    if (i < state->next_free_grain) {
      state->next_free_grain = i;
    }
  } else {
    // Apply simple linear envelope
    double env_pos = (double)(state->positions[i]) / state->lengths[i];
    if (env_pos < state->overlap) {
      state->amps[i] = env_pos / state->overlap;
    } else if (env_pos > (1.0 - state->overlap)) {
      state->amps[i] = (1.0 - env_pos) / state->overlap;
    } else {
      state->amps[i] = 1.0;
    }
  }
}

node_perform granulator_perform(Node *node, int nframes, double spf) {
  granulator_state *state = node->state;

  double *buf = node->ins[0].buf;
  int buf_size = node->ins[0].size;

  double *trig = node->ins[1].buf;
  double *pos = node->ins[2].buf;
  double *rate = node->ins[3].buf;
  double *out = node->out.buf;

  while (nframes--) {
    *out = 0.0;
    double p = fabs(*pos);

    if (*trig == 1.0 && state->active_grains < state->max_concurrent_grains) {
      int start_pos = (int)(p * buf_size);
      int length = state->min_grain_length +
                   rand() % (state->max_grain_length - state->min_grain_length);

      // Initialize new grain
      int index = state->next_free_grain;
      init_grain(state, index, start_pos, length, *rate);

      // Find the next free grain
      do {
        state->next_free_grain =
            (state->next_free_grain + 1) % state->max_concurrent_grains;
      } while (state->lengths[state->next_free_grain] != 0 &&
               state->next_free_grain != index);
    }

    // Process active grains
    for (int i = 0; i < state->max_concurrent_grains; i++) {
      if (state->lengths[i] > 0) {
        process_grain(state, i, out, buf, buf_size);
      }
    }
    // Normalize output
    if (state->active_grains > 0) {
      *out /= state->active_grains;
    }

    trig++;
    pos++;
    rate++;
    out++;
  }
}

Node *granulator_node(int max_concurrent_grains, Signal *buf, Signal *trig,
                      Signal *pos, Signal *rate) {
  granulator_state *state = malloc(sizeof(granulator_state));
  void *grain_state_arrays = malloc(3 * max_concurrent_grains * sizeof(int) +
                                    2 * max_concurrent_grains * sizeof(double));

  state->starts = grain_state_arrays;
  grain_state_arrays += (max_concurrent_grains * sizeof(int));
  state->lengths = grain_state_arrays;
  grain_state_arrays += (max_concurrent_grains * sizeof(int));
  state->positions = grain_state_arrays;
  grain_state_arrays += (max_concurrent_grains * sizeof(int));

  state->rates = grain_state_arrays;
  grain_state_arrays += (max_concurrent_grains * sizeof(double));
  state->amps = grain_state_arrays;
  grain_state_arrays += (max_concurrent_grains * sizeof(double));

  state->max_concurrent_grains = max_concurrent_grains;
  state->active_grains = 0;
  state->min_grain_length = 7000;
  state->max_grain_length = 7001;
  state->overlap = 0.3;

  Signal *sigs = malloc(sizeof(Signal) * 4);
  sigs[0].buf = buf->buf;
  sigs[0].size = buf->size;
  sigs[0].layout = buf->layout;

  sigs[1].buf = trig->buf;
  sigs[1].size = trig->size;
  sigs[1].layout = trig->layout;

  sigs[2].buf = pos->buf;
  sigs[2].size = pos->size;
  sigs[2].layout = pos->layout;

  sigs[3].buf = rate->buf;
  sigs[3].size = rate->size;
  sigs[3].layout = rate->layout;

  Node *granulator =
      node_new(state, (node_perform *)granulator_perform, 4, sigs);

  return granulator;
}
