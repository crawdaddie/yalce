#include "../audio/node.h"
#include "../user_ctx.h"
#include "../util.c"
#include <pthread.h>
#include <sndfile.h>

Node *get_square_synth_node(double freq, double *bus, long sustain) {

  Node *head = get_sin_detune_node(freq);
  Node *tail = head;
  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);
  tail =
      node_add_to_tail(get_biquad_lpf(tail->out, 2500, 0.2, 2.0, 48000), tail);
  Node *env = get_env_node(10, 25.0, (double)sustain);

  tail = node_mul(env, tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(bus));
  data->graph = head;
  data->bus = bus;

  Node *out_node =
      alloc_node((NodeData *)data, NULL, (t_perform)perform_synth_graph,
                 "synth", (t_free_node)free_synth);
  env_set_on_free(env, out_node, on_env_free);
  return out_node;
}
Node *get_synth_graph(double *bus) {
  int sample_rate = 48000;
  Node *head = alloc_node(NULL, NULL, (t_perform)perform_null, "head", NULL);
  Node *tail = head;

  /* tail = */
  /*     node_add_to_tail(get_delay_node(bus, 750, 1000, 0.3, sample_rate),
   * tail); */
  tail->out = bus;

  return head;
}

Node *ctx_play_synth(UserCtx *ctx, Node *group, double freq, long sustain,
                     double schedule_at) {
  Node *head = group;
  Node *next = head->next;
  Node *synth = node_add_to_tail(
      get_square_synth_node(freq, ctx->buses[0], sustain), head);
  synth->next = next;
  synth->schedule = schedule_at;
  return synth;
};

static double pitches[7] = {261.626,         311.127, 349.228, 391.995,
                            415.30469757995, 466.164, 523.251};

static double octaves[5] = {0.125, 0.25, 0.5, 1, 2.0};

void *modulate_pitch(void *arg) {
  struct player_ctx *player_ctx = (struct player_ctx *)arg;
  UserCtx *ctx = player_ctx->ctx;
  Node *group = player_ctx->group;
  free(player_ctx);

  long msec = 500;
  double last_tick = ctx->seconds_offset;

  for (;;) {
    int p_index = rand() % 7;
    int o_index = rand() % 4;
    double p = pitches[p_index] * octaves[o_index];
    double secs = (double)msec / 1000;
    double schedule_at = last_tick + secs + ctx->latency;
    /* long msec = 1000 * octaves[rand() % 5]; */
    last_tick = ctx->seconds_offset;
    Node *synth = ctx_play_synth(ctx, group, p, msec, schedule_at);
    sleep_millisecs(msec);
  }
}

void attach_synth_thread(UserCtx *ctx) {
  struct player_ctx *player_ctx = get_player_ctx_ref(ctx);

  /* Node *tail = node_add_to_tail( */
  /*     get_delay_node(ctx->buses[0], 750, 1000, 0.3, 48000),
   * player_ctx->group); */
  /* tail->out = ctx->buses[0]; */

  pthread_t thread;
  pthread_create(&thread, NULL, modulate_pitch, (void *)player_ctx);
}
