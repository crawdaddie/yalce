#include "../audio/node.h"
#include "../scheduling.c"
#include "../user_ctx.h"
#include <pthread.h>
#include <sndfile.h>
#include <sys/time.h>

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
                           long sustain, double schedule_at) {
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
  out_node->schedule = schedule_at;
  debug_node(out_node, "new kick");
  set_on_free_handler(env, out_node, on_env_free);
  return out_node;
}

Node *ctx_play_kick(UserCtx *ctx, Node *group, struct buf_info *b_info,
                    double rate, long sustain, double schedule_at) {
  Node *head = group;
  Node *next = head->next;
  Node *synth = node_add_to_tail(
      get_kick_sample_node(b_info->data, b_info->frames, ctx->buses[0], rate,
                           sustain, schedule_at),
      head);
  synth->next = next;
  return synth;
}

static double kick_rates[4] = {1, 1, 1, 1};
static double kick_times[4] = {0.25, 0.5, 1, 1};
void *kicks(void *arg) {

  struct player_ctx *player_ctx = (struct player_ctx *)arg;
  struct timespec initial_time = player_ctx->initial_time;
  printf("thread start %d.%.9ld\n", (int)initial_time.tv_sec,
         initial_time.tv_nsec);
  UserCtx *ctx = player_ctx->ctx;
  Node *group = player_ctx->group;
  free(player_ctx);
  double last_tick = ctx->seconds_offset;

  struct buf_info *b_info = read_sndfile("kick.wav");

  for (;;) {
    double rate = kick_rates[rand() % 4];

    struct timespec thread_time;
    sub_timespec(initial_time, get_time(), &thread_time);

    double secs = timespec_to_secs(thread_time);
    printf("kick thread time %d.%.9ld secs %lf\n", (int)thread_time.tv_sec,
           thread_time.tv_nsec, secs);

    double schedule_at = ctx->seconds_offset + ctx->latency;
    /* long msec = 500 * kick_times[rand() % 4]; */
    long msec = 500;
    ctx_play_kick(ctx, group, b_info, 1.0, msec, schedule_at);
    msleep(msec);
    last_tick += 500;
  }
}

void attach_kick_thread(UserCtx *ctx, struct timespec initial_time) {
  struct player_ctx *player_ctx = get_player_ctx_ref(ctx, initial_time);

  pthread_t thread;
  pthread_create(&thread, NULL, kicks, (void *)player_ctx);
}
