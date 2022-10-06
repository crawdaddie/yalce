#include "../audio/node.h"
#include "../user_ctx.h"
#include "../util.c"
#include <pthread.h>
#include <sndfile.h>
#include <sys/time.h>

Node *get_square_synth_node(double freq, double *bus, long sustain) {

  Node *head = get_sq_detune_node(freq);
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
  set_on_free_handler(env, out_node, on_env_free);
  return out_node;
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
double get_time() {
  struct timeval thread_tick;
  gettimeofday(&thread_tick, NULL);
  return (double)thread_tick.tv_sec + (double)thread_tick.tv_usec / 1000;
}
void *modulate_pitch(void *arg) {

  struct player_ctx *player_ctx = (struct player_ctx *)arg;
  UserCtx *ctx = player_ctx->ctx;
  Node *group = player_ctx->group;
  free(player_ctx);
  double last_tick = ctx->seconds_offset;

  long msec = 500;

  for (;;) {
    int p_index = rand() % 7;
    int o_index = rand() % 4;
    double p = pitches[p_index] * octaves[o_index];
    double secs = (double)msec / 1000;
    double schedule_at = last_tick + secs + ctx->latency;
    last_tick = ctx->seconds_offset;
    Node *synth = ctx_play_synth(ctx, group, p, msec, 0.0);
    debug_node(synth, "playing new synth");
    sleep_millisecs(msec);
  }
}

void attach_synth_thread(UserCtx *ctx) {
  struct player_ctx *player_ctx = get_player_ctx_ref(ctx);

  Node *tail = node_add_to_tail(
      get_delay_node(ctx->buses[0], 750, 1000, 0.3, 48000), player_ctx->group);
  tail->out = ctx->buses[0];

  pthread_t thread;
  pthread_create(&thread, NULL, modulate_pitch, (void *)player_ctx);
}
