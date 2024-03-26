#include "entry.h"
#include "ctx.h"
#include "delay.h"
#include "envelope.h"
#include "node.h"
#include "noise.h"
#include "oscillator.h"
#include "scheduling.h"
#include "signal.h"
#include "signal_arithmetic.h"
#include "window.h"
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
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

int get_block_offset() {

  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  return offset;
}

void set_node_scalar_at(Node *target, int offset, int input, double value) {

  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_trig_at(Node *target, int offset, int input) {
  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_scalar(Node *target, int input, double value) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_trig(Node *target, int input) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void push_msgs(int num_msgs, scheduler_msg *scheduler_msgs) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  for (int i = 0; i < num_msgs; i++) {
    printf("push msgs %d %p type: %d\n", offset, scheduler_msgs + i,
           (scheduler_msgs + i)->type);
    scheduler_msg msg = scheduler_msgs[i];
    msg.frame_offset = offset;
    push_msg(&ctx->msg_queue, msg);
  }
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
void cleanup_job() {
  while (true) {
    Ctx *ctx = get_audio_ctx();
    // printf("cleanup thread\n");
    cleanup_graph(&ctx->graph, ctx->graph.head);
    msleep(250);
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

int entry() {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void *)audio_entry, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // Raylib wants to be in the main thread :(
  pthread_t cleanup_thread;
  if (pthread_create(&thread, NULL, (void *)cleanup_job, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  create_spectrogram_window();
  pthread_join(cleanup_thread, NULL);
  pthread_join(thread, NULL);
}
