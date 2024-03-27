#include "entry.h"
#include "ctx.h"
#include "envelope.h"
#include "node.h"
#include "noise.h"
#include "oscillator.h"
#include "scheduling.h"
#include "signal.h"
#include "signal_arithmetic.h"
#include "window.h"
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

Node *cleanup_graph(Graph *graph, Node *head) {
  if (!head) {
    return NULL;
  };

  Node *next = head->next;
  if (head->killed) {
    // head->perform(head, nframes, seconds_per_frame);
    if (head->is_group) {
      cleanup_graph((Graph *)head->state, ((Graph *)head->state)->head);
    }
    graph_delete_node(graph, head);
  }

  if (next) {
    return cleanup_graph(graph, next);
  };
  return NULL;
}
void audio_ctx_gc() {
  while (true) {
    Ctx *ctx = get_audio_ctx();
    // printf("cleanup thread\n");
    cleanup_graph(&ctx->graph, ctx->graph.head);
    msleep(250);
  }
}
void cleanup_job() {
  pthread_t cleanup_thread;
  if (pthread_create(&cleanup_thread, NULL, (void *)audio_ctx_gc, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
  }
}

static double levels[3] = {0.0, 1.0, 0.0};
static double times[2] = {0.001, 0.8};
Node *synth(double freq) {
  Node *group = group_new();
  group->num_ins = 1;
  group->ins = malloc(sizeof(Signal *));
  group->ins[0] = get_sig_default(1, 0);

  Node *sq = sq_node(freq);
  group_add_tail(group, sq);
  Node *env = env_node(2, levels, times);
  pipe_sig_to_idx(0, group->ins[0], env);

  set_node_trig(group, 0);
  group_add_tail(group, env);

  sq = mul_nodes(env, sq);
  group_add_tail(group, sq);

  add_to_dac(sq);
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

void *audio_entry() {
  while (true) {
    Node *sq = synth(choices[rand_int(8)] / 2.0);
    add_to_dac(sq);
    ctx_add(sq);
    printf("----------\n");
    graph_print(&(get_audio_ctx()->graph), 0);

    msleep(250);
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

  create_spectrogram_window();
  pthread_join(cleanup_thread, NULL);
  pthread_join(thread, NULL);
}
