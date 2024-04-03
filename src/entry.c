#include "entry.h"
#include "common.h"
#include "ctx.h"
#include "envelope.h"
#include "gc.h"
#include "node.h"
#include "scheduling.h"
#include "signal.h"
#include "signal_arithmetic.h"
#include "soundfile.h"
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
static double times[2] = {0.001, 0.8};

Node *synth(Signal *bufnum) {
  Node *group = group_new(2);
  // group->num_ins = 1;
  // group->ins = malloc(sizeof(Signal *));
  // group->ins[0] = get_sig_default(1, 0);

  Node *b = bufplayer_autotrig_node(bufnum, 1.0, 0.);
  group_add_tail(group, b);
  Node *env = autotrig_env_node(2, levels, times);
  pipe_sig_to_idx(0, group->ins[0], env);

  // set_node_trig(group, 0);
  group_add_tail(group, env);

  b = mul_nodes(b, env);
  group_add_tail(group, b);

  add_to_dac(b);
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
void *audio_entry() {
  Signal *sig = malloc(sizeof(Signal));
  // int bufnum = buf_alloc("assets/sor_sample_pack/Kicks/SOR_OB_Kick_Mid.wav");
  // int buf_sr;
  // read_file("assets/sor_sample_pack/Kicks/SOR_OB_Kick_Mid.wav", sig,
  // &buf_sr);
  write_sig(sig);
  while (true) {
    Node *sq = synth(sig);
    int offset = get_block_offset();
    add_node_msg(sq, offset);
    // add_to_dac(sq);
    // ctx_add(sq);
    // printf("----------\n");
    graph_print(&(get_audio_ctx()->graph), 0);

    msleep(1000);
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
  pthread_join(cleanup_thread, NULL);
  pthread_join(thread, NULL);
}
