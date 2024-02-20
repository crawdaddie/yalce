#include "entry.h"
#include "common.h"
#include "node.h"
#include "raylib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline void underguard(double *x) {
  union {
    u_int32_t i;
    double f;
  } ix;
  ix.f = *x;
  if ((ix.i & 0x7f800000) == 0)
    *x = 0.0f;
}

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
      *out->buf = random_double();
      out->buf++;
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
  sin_state *state = node->state;
  double *out = node->out->buf;
  double *input = node->ins->buf;

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

Node *sine(double freq) {
  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Signal *in = get_sig(1);
  for (int i = 0; i < in->size; i++) {
    in->buf[i] = freq;
  }

  Node *s = node_new(state, (node_perform *)sine_perform, in, get_sig(1));
  s->ins = in;
  return s;
}

// ----------------------------- SQUARE WAVE OSCILLATORS
//
double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

node_perform sq_perform(Node *node, int nframes, double spf) {
  sq_state *state = node->state;

  double *out = node->out->buf;
  double *in = node->ins->buf;

  while (nframes--) {
    double samp =
        sq_sample(state->phase, *in) + sq_sample(state->phase, *in * 1.01);
    samp /= 2;

    state->phase += spf;
    *out = samp;
    out++;
    in++;
  }
}

Node *sq_node(double freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;
  Signal *in = get_sig(1);
  int n = in->size;
  for (int i = 0; i < n; i++) {
    in->buf[i] = freq;
  }

  Node *s = node_new(state, sq_perform, in, get_sig(1));
  s->ins = in;
  return s;
}

node_perform lf_noise_perform(Node *node, int nframes, double spf) {
  lf_noise_state *state = node->state;

  double noise_freq = state->freq;
  int trigger_per_ms = (int)(1000 / noise_freq);
  double *out = node->out->buf;
  // Signal in = node->ins;

  while (nframes--) {

    if ((int)(1000 * state->phase) >= trigger_per_ms) {
      state->phase = 0;
      state->target = random_double_range(state->min, state->max);
      // printf("noise target: %f [%f, %f]\n", data->target, data->min,
      // data->max);
    }

    state->phase += spf;
    *out = state->target;
    out++;
    // in.data++;
  }
}

// ------------------------------------------- DISTORTION
typedef struct {
  double gain;
} tanh_state;

node_perform tanh_perform(Node *node, int nframes, double spf) {
  tanh_state *state = node->state;
  double *in = node->ins->buf;
  double *out = node->out->buf;
  while (nframes--) {
    *out = tanh(state->gain * (*in));
    in++;
    out++;
  }
}

Node *tanh_node(double gain, Node *node) {

  tanh_state *state = malloc(sizeof(tanh_state));
  state->gain = gain;
  Signal *in = node->out;
  Signal *out = node->out;

  Node *s = node_new(state, tanh_perform, in, out);
  return s;
}

// ------------------------------------------- FILTERS
//
typedef struct {
  double *buf;
  int read_pos;
  int write_pos;
  int buf_size;
  double fb;
} comb_state;

static inline void comb_perform_tick(comb_state *state, double *in,
                                     double *out) {
  // *out =
}

node_perform comb_perform(Node *node, int nframes, double spf) {
  double *in = node->ins->buf;
  double *out = node->out->buf;
  comb_state *state = node->state;

  double *write_ptr = (state->buf + state->write_pos);
  double *read_ptr = state->buf + state->read_pos;

  while (nframes--) {
    write_ptr = state->buf + state->write_pos;
    read_ptr = state->buf + state->read_pos;

    *out = *in + *read_ptr;

    *write_ptr = state->fb * (*out);

    state->read_pos = (state->read_pos + 1) % state->buf_size;
    state->write_pos = (state->write_pos + 1) % state->buf_size;
    in++;
    out++;
  }
}

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input) {
  Ctx *ctx = get_audio_ctx();
  int SR = ctx->SR;
  comb_state *state = malloc(sizeof(comb_state));
  int buf_size = (int)max_delay_time * SR;

  state->read_pos = buf_size - (int)(delay_time * SR);
  state->write_pos = 0;
  state->buf_size = buf_size;
  state->fb = fb;

  double *buf = malloc(sizeof(double) * (int)max_delay_time * SR);
  double *b = buf;

  while (buf_size--) {
    *b = 0.0;
    b++;
  }

  state->buf = buf;
  Node *s = node_new(state, comb_perform, input->out, get_sig(1));
  return s;
}

// ------------------------------------------- NOISE UGENS

Node *lfnoise(double freq, double min, double max) {
  lf_noise_state *state = malloc(sizeof(lf_noise_state));
  state->phase = 0.0;
  state->target = random_double_range(min, max);
  state->min = min;
  state->max = max;
  state->freq = freq;

  return node_new(state, lf_noise_perform, &(Signal){}, get_sig(1));
}

node_perform lf_noise_interp_perform(Node *node, int nframes, double spf) {
  double noise_freq = 5.0;
  int trigger_per_ms = (int)(1000 / noise_freq);

  lf_noise_interp_state *state = node->state;

  // printf("noise: %f %f\n", data->val, data->phase);

  double *out = node->out->buf;

  while (nframes--) {

    // printf("noise: %f %d thresh: %d\n", data->target, (int)data->phase,
    // trigger_per_ms);
    if ((int)(1000 * state->phase) >= trigger_per_ms) {
      state->phase = 0;
      state->target = random_double_range(60, 120);
      // printf("noise: %f\n", data->target);
    }

    state->phase += spf;
    *out = state->current;
    state->current =
        state->current + 0.000001 * (state->target - state->current);
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
  sig->buf = buf_ptr;
  sig->layout = layout;
  sig->size = BUF_SIZE;
  buf_ptr += BUF_SIZE * layout;
  return sig;
}
// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
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

void *win(void) {
  int screen_width = 400;
  int screen_height = 400;
  InitWindow(screen_width, screen_height,
             "raylib [core] example - basic window");
  Vector2 position[LAYOUT_CHANNELS] = {0, 0};
  // double prev_pos_y[LAYOUT_CHANNELS] = {0};

  Vector2 prev_pos[LAYOUT_CHANNELS] = {0, 0};
  SetTargetFPS(30); // Set our game to run at 30 frames-per-second
  //
  Ctx *ctx = get_audio_ctx();

  double *data = ctx->dac_buffer.buf;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw the current buffer state proportionate to the screen
    prev_pos[0].x = 0.;
    prev_pos[1].x = 0.;

    for (int i = 0; i < screen_width; i++) {
      position[0].x = (float)i;
      position[1].x = (float)i;
      for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
        position[ch].y = (2 * ch + 1) * screen_height / (LAYOUT_CHANNELS * 2) +
                         10 * data[ch + i * BUF_SIZE / screen_width];
        DrawLine(prev_pos[ch].x, prev_pos[ch].y, position[ch].x, position[ch].y,
                 RED);
        prev_pos[ch] = (Vector2){(float)(i), position[ch].y};
      }
    }
    EndDrawing();
  }
  CloseWindow();
}

void *audio_entry() {
  // Create a new thread for the window loop
  Node *chain = chain_new();
  Node *noise = add_to_chain(chain, lfnoise(8., 80., 500.));
  Node *sig = add_to_chain(chain, sine(100.0));
  sig = pipe_output(noise, sig);
  sig = add_to_chain(chain, tanh_node(8.0, sig));
  sig = add_to_chain(chain, comb_node(0.0, 1.0, 0.9, sig));

  add_to_dac(chain);
  ctx_add(chain);
  //
  // sleep(10);
}

int entry() {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void *)audio_entry, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  // Raylib wants to be in the main thread :(
  win();

  pthread_join(thread, NULL);
}
