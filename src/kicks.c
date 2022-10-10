#include "audio/graph.h"
#include "config.h"
#include <stdlib.h>
#include "user_ctx.h"

typedef void (*t_trigger)(void *data, nframes_t i);
typedef struct kick_data {
  sample_t *out;
  sample_t ramp;
  sample_t freq;
  int trigger_frame; // set this to a sample offset within a block to retrigger
                     // the node - reset back to -1 immediately after to avoid
                     // triggering every block
} kick_data;

void process_triggers(kick_data *data, nframes_t i) {
  if (data->trigger_frame != -1 && i == data->trigger_frame) {
    data->ramp = 0.0;
    data->trigger_frame = -1;
  }
}

sample_t ramp_to_pitch_env(sample_t ramp) { return 1000 * (1 - ramp); }


sample_t pitch_env(sample_t i, sample_t decay) {
  return 1.0 / (1.0 + decay * i);
}
sample_t scale_val(sample_t env_val, // 0-1
                         sample_t min, sample_t max) {
  return max + env_val * (max - min);
}

t_perform perform_kick(Graph *graph, nframes_t nframes) {
  kick_data *data = (kick_data *)graph->data;

  for (int i = 0; i < nframes; i++) {
    process_triggers(data, i);

    sample_t env_val = scale_val(pitch_env(data->ramp, 0.02), data->freq, 4000.0);
    sample_t sample = sin(env_val * 2.0 * PI);
    sample += sin(env_val * 1.01 * 2.0 * PI);
    sample += scale_val((sample_t)rand() / RAND_MAX, -1, 1) * sin(data->ramp * 30 * 2.0 * PI) * 0.04;
    sample += tanh(sample * 5.0);
    graph->out[i] = sample * 0.125;
    data->ramp += (sample_t)1.0 / 48000;
  }
}

void set_kick_trigger(Graph *kick_node, int time, Graph *kick_node_ref) {
  kick_data *data = (kick_data *)kick_node_ref->data;
  data->trigger_frame = time;
}

void make_kick(Graph *graph, int time, void *ref) {
  kick_data *data = malloc(sizeof(kick_data));
  data->out = calloc(BUF_SIZE, sizeof(sample_t));
  data->ramp = 0.0;
  data->trigger_frame = -1;
  data->freq = 60.0;
  sample_t *out = calloc(BUF_SIZE, sizeof(sample_t));
  Graph *kick_node = malloc(sizeof(Graph) + sizeof(out) + sizeof(data));
  kick_node->data = (NodeData *)data;
  kick_node->name = "kick";
  kick_node->perform = (t_perform)perform_kick;
  kick_node->prev = NULL;
  kick_node->next = NULL;
  kick_node->should_free = 0;
  kick_node->schedule = 0;
  kick_node->out = out;
  add_after(graph, kick_node);
}

void add_kick_node(jack_client_t *client, UserCtx *ctx) {
  jack_nframes_t frame_time = jack_frames_since_cycle_start(client);
  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "kick node";
  msg->time = frame_time;
  msg->func = make_kick;
  msg->ref = NULL;
  enqueue(ctx->msg_queue, msg);
}

void trigger_kick_node(jack_client_t *client, UserCtx *ctx, Graph *node) {

  jack_nframes_t frame_time = jack_frames_since_cycle_start(client);

  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "kick node trig";
  msg->time = frame_time;
  msg->func = (Action)set_kick_trigger;
  msg->ref = node;
  enqueue(ctx->msg_queue, msg);
}
