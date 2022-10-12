#include "audio/graph.h"
#include "config.h"
#include "user_ctx.h"
#include <stdlib.h>

typedef void (*t_trigger)(void *data, nframes_t i);
typedef struct kick_data {
  sample_t ramp;
  sample_t freq;
  int t_ramp; // set this to a sample offset within a block to retrigger
              // the node - reset back to -1 immediately after to avoid
              // triggering every block
} kick_data;

kick_data *alloc_kick_data() {
  kick_data *data = malloc(sizeof(kick_data));
  data->ramp = 0.0;
  data->t_ramp = -1;
  data->freq = 60.0;
  return data;
}

void process_triggers(kick_data *data, nframes_t i) {
  if (data->t_ramp != -1 && i == data->t_ramp) {
    data->ramp = 0.0;
    data->t_ramp = -1;
  }
}
/* int process_schedule(Graph *node, nframes_t i) { */
/*   if (node->schedule == -1) { */
/*     return 0; */
/*   } */
/*   if (i < node->schedule && !(node->schedule_change_data)) { */
/*     return 1; */
/*   } */
/*   if (i == node->schedule) { */
/*     node->schedule = -1; */
/*     return 0; */
/*   } */
/* } */

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
    if (i < graph->schedule) {
      graph->schedule = -1;
      break;
    };
    process_triggers(data, i);

    sample_t env_val =
        scale_val(pitch_env(data->ramp, 0.02), data->freq, 4000.0);
    sample_t sample = sin(env_val * 2.0 * PI);
    sample += sin(env_val * 1.01 * 2.0 * PI);
    sample += scale_val((sample_t)rand() / RAND_MAX, -1, 1) *
              sin(data->ramp * 30 * 2.0 * PI) * 0.04;
    sample += tanh(sample * 5.0);
    graph->out[i] = sample * 0.125;
    data->ramp += (sample_t)1.0 / 48000;
  }
}


void set_kick_freq(Graph *kick_node, int time, Graph *kick_node_ref) {
  kick_data *data = (kick_data *)kick_node_ref->data;
  data->t_ramp = time;
}

void add_kick_node_msg_handler(Graph *graph, int time, void **args) {
  kick_data *data = alloc_kick_data();
  sample_t *outbus = args[0];
  Graph *kick_node = alloc_graph((NodeData *)data, NULL, perform_kick);
  kick_node->out = outbus;
  add_after(graph, kick_node);
}

void add_kick_node_msg(jack_client_t *client, UserCtx *ctx) {
  jack_nframes_t frame_time = jack_frames_since_cycle_start(client);
  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "kick node";
  msg->time = frame_time;
  msg->func = (Action)add_kick_node_msg_handler;
  msg->ref = NULL;


  msg->num_args = 1;
  msg->args = malloc(msg->num_args * sizeof(void *));
  msg->args[0] = ctx->buses[0];
  enqueue(ctx->msg_queue, msg);
}

void trigger_kick_node_msg_handler(Graph *kick_node, int time, void **args) {
  Graph *kick_node_ref = args[0];
  kick_data *data = (kick_data *)kick_node_ref->data;
  kick_node->schedule = time;
  data->t_ramp = time;
}
void trigger_kick_node_msg(jack_client_t *client, UserCtx *ctx, Graph *node) {

  jack_nframes_t frame_time = jack_frames_since_cycle_start(client);

  queue_msg_t *msg = malloc(sizeof(queue_msg_t));
  msg->msg = "kick node trig";
  msg->time = frame_time;
  msg->func = (Action)trigger_kick_node_msg_handler;

  msg->num_args = 1;
  msg->args = malloc(msg->num_args * sizeof(void *));
  msg->args[0] = node;
  enqueue(ctx->msg_queue, msg);
}
