#include "audio/node.h"
#include "user_ctx.h"
#include "util.c"
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

  tail =
      node_add_to_tail(get_delay_node(bus, 750, 1000, 0.3, sample_rate), tail);
  tail->out = bus;

  return head;
}

Node *ctx_play_synth(UserCtx *ctx, Node *group, double freq, long sustain) {
  Node *head = group;
  Node *next = head->next;
  Node *synth = node_add_to_tail(
      get_square_synth_node(freq, ctx->buses[0], sustain), head);
  synth->next = next;
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

  for (;;) {
    int p_index = rand() % 7;
    int o_index = rand() % 4;
    double p = pitches[p_index] * octaves[o_index];
    long msec = 1000 * octaves[rand() % 5];
    Node *synth = ctx_play_synth(ctx, group, p, msec);

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

struct buf_info {
  double *data;
  int frames;
};

struct buf_info *read_sndfile(char *filename) {
  SNDFILE *file;
  int fs;
  SF_INFO file_info;

  file = sf_open(filename, SFM_READ, &file_info);
  double *buf = calloc(24000, sizeof(double));

  sf_count_t frames_read = sf_readf_double(file, buf, 24000);
  struct buf_info *b_info = malloc(sizeof(struct buf_info));
  b_info->data = buf;
  b_info->frames = frames_read;
  return b_info;
}

Node *get_kick_sample_node(double *buf, int frames, double *bus, double rate,
                           long sustain) {
  Node *head = get_bufplayer_interp_node(buf, frames, rate, 0.0, 0);
  Node *tail = head;

  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);

  Node *env = get_env_node(10, (double)sustain, 100);
  tail = node_mul(env, tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(bus));
  data->graph = head;
  data->bus = bus;

  Node *out_node =
      alloc_node((NodeData *)data, NULL, (t_perform)perform_synth_graph, "kick",
                 (t_free_node)free_synth);
  env_set_on_free(env, out_node, on_env_free);
  return out_node;
}

Node *ctx_play_kick(UserCtx *ctx, Node *group, struct buf_info *b_info,
                    double rate, long sustain) {
  Node *head = group;
  Node *next = head->next;
  Node *synth =
      node_add_to_tail(get_kick_sample_node(b_info->data, b_info->frames,
                                            ctx->buses[0], rate, sustain),
                       head);
  synth->next = next;
  synth->schedule = ctx->seconds_offset + ctx->latency;
  return synth;
}

static double kick_rates[4] = {1, 1, 1, 1};
void *kicks(void *arg) {
  struct player_ctx *player_ctx = (struct player_ctx *)arg;
  UserCtx *ctx = player_ctx->ctx;
  Node *group = player_ctx->group;
  free(player_ctx);
  struct buf_info *b_info = read_sndfile("kick.wav");

  for (;;) {
    long msec = 500;
    double rate = kick_rates[rand() % 4];
    ctx_play_kick(ctx, group, b_info, 240 / 180, msec);
    sleep_millisecs(msec);
  }
}

void attach_kick_thread(UserCtx *ctx) {
  /* struct player_ctx *player_ctx = get_player_ctx_ref(ctx); */
  /*  */
  /* pthread_t thread; */
  /* pthread_create(&thread, NULL, kicks, (void *)player_ctx); */
}
