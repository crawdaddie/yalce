#include "entry.h"
#include "biquad.h"
#include "common.h"
#include "ctx.h"
#include "envelope.h"
#include "gc.h"
#include "node.h"
#include "noise.h"
#include "oscillator.h"
#include "scheduling.h"
#include "signal.h"
#include "signal_arithmetic.h"
#include "window.h"
#include <math.h>
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

static double levels[3] = {0.0, 1.0, 0.0};
static double times[2] = {0.2, 0.5};

Node *synth(double freq, double cutoff) {
  Node *group = group_new(1);
  group->num_ins = 0;
  // group->ins = malloc(sizeof(Signal *) * 2);
  // group->ins[0] = get_sig_default(1, 100.);
  // group->ins[1] = get_sig_default(1, 100.);

  Node *sq1 = sq_node(freq);
  group_add_tail(group, sq1);
  // node_set_input_signal(sq1, 0, group->ins[0]);

  Node *sq2 = sq_node(freq * 1.01);
  group_add_tail(group, sq2);
  // node_set_input_signal(sq2, 0, group->ins[0]);

  Node *nodes[2] = {sq1, sq2};

  Node *summed = sum_nodes_arr(2, nodes);
  group_add_tail(group, summed);

  Node *filtered = biquad_lp_dyn_node(cutoff, 1., summed);
  group_add_tail(group, filtered);
  // node_set_input_signal(filtered, 1, group->ins[1]);

  Node *env = autotrig_env_node(2, levels, times);
  group_add_tail(group, env);

  Node *m = mul_nodes(filtered, env);

  group_add_tail(group, m);

  add_to_dac(m);
  return group;
}

static double choices[8] = {220.0,
                            246.94165062806206,
                            261.6255653005986,
                            293.6647679174076,
                            329.6275569128699,
                            349.2282314330039,
                            391.99543598174927,
                            880.0};

void write_sig(Signal *sig) {
  sig->buf = calloc(sizeof(double), 48000 * 2);
  sig->layout = 2;
  sig->size = 48000;
  double phase = 0.0;
  double phsinc = (2. * PI) / sig->size;

  for (int i = 0; i < sig->size; i++) {
    double val = sin(phase);

    // printf("%f\n", val);
    sig->buf[2 * i] = val;
    sig->buf[2 * i + 1] = val;
    phase += phsinc;
  }
}

void compile_synth(const char *name, Node *synth_graph) {
  printf("compile bin:\n");
  Graph g = {.head = synth_graph};
  graph_print(&g, 0);
}

void *audio_entry() {
  // Node *proto = synth();
  // compile_synth("sq_env_simple", proto);

  // while (true) {
  //
  //   double freq = choices[rand_int(8)];
  //
  //   double cutoff = choices[rand_int(8)];
  //   // choices[rand_int(8)];
  //   Node *sq = synth(freq, cutoff);
  //   int offset = get_block_offset();
  //   add_node_msg(sq, offset);
  //   // set_node_scalar_at(sq, offset, 0, choices[rand_int(8)]);
  //   // add_to_dac(sq);
  //   // ctx_add(sq);
  //   // printf("----------\n");
  //   // graph_print(&(get_audio_ctx()->graph), 0);
  //
  //   msleep(250);
  // }
  //
  // Node *env = env_node(2, levels, times);
  // add_to_dac(env);
  // ctx_add(env);
  // set_node_trig(env, 0);
  // msleep(1000);
  while (true) {
    msleep(500);
  }
}
void print_ctx() { graph_print(&(get_audio_ctx()->graph), 0); }

int entry() {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void *)audio_entry, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // Raylib wants to be in the main thread :(
  pthread_t cleanup_thread;
  if (pthread_create(&thread, NULL, (void *)audio_ctx_gc, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  // create_spectrogram_window();

  create_window();
  pthread_join(cleanup_thread, NULL);
  pthread_join(thread, NULL);
}
