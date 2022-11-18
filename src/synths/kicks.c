#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include <stdlib.h>

typedef void (*t_trigger)(void *data, t_nframes i);
typedef struct kick_data {
  t_sample ramp;
  t_sample freq;
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

void process_triggers(kick_data *data, t_nframes i) {
  if (data->t_ramp != -1 && i == data->t_ramp) {
    data->ramp = 0.0;
    data->t_ramp = -1;
  }
}
/* int process_schedule(Graph *node, t_nframes i) { */
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

t_sample pitch_env(t_sample i, t_sample decay) {
  return 1.0 / (1.0 + decay * i);
}
t_sample scale_val(t_sample env_val, // 0-1
                   t_sample min, t_sample max) {
  return min + env_val * (max - min);
}

t_perform perform_kick(Graph *graph, t_nframes nframes) {
  kick_data *data = (kick_data *)graph->data;

  for (int i = 0; i < nframes; i++) {
    if (i < graph->schedule) {
      break;
    };
    graph->schedule = -1;
    process_triggers(data, i);

    t_sample env_val =
        scale_val(pitch_env(data->ramp, 0.02), data->freq, 4000.0);
    t_sample sample = sin(env_val * 2.0 * PI);
    sample += scale_val((t_sample)rand() / RAND_MAX, -1, 1) *
              sin(data->ramp * 30 * 2.0 * PI) * 0.04;
    sample += tanh(sample * 5.0);
    graph->out[i] = sample * 0.125;
    data->ramp += (t_sample)1.0 / 48000;
  }
}

void set_kick_freq(Graph *kick_node, int time, Graph *kick_node_ref) {
  kick_data *data = (kick_data *)kick_node_ref->data;
  data->t_ramp = time;
}

void add_kick_node_msg_handler(Graph *graph, int time, void **args) {
  kick_data *data = alloc_kick_data();
  t_sample *outbus = args[0];
  Graph *kick_node =
      alloc_graph((NodeData *)data, outbus, (t_perform)perform_kick, 1);
  add_after(graph, kick_node);
}

void add_kick_node_msg(UserCtx *ctx, t_nframes frame_time, t_sample freq) {
  t_queue_msg *msg = malloc(sizeof(t_queue_msg));
  msg->msg = "kick node";
  msg->time = frame_time;
  msg->func = (MsgAction)add_kick_node_msg_handler;
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
void trigger_kick_node_msg(UserCtx *ctx, Graph *node, t_nframes frame_time) {

  t_queue_msg *msg = malloc(sizeof(t_queue_msg));
  msg->msg = "kick node trig";
  msg->time = frame_time;
  msg->func = (MsgAction)trigger_kick_node_msg_handler;

  msg->num_args = 1;
  msg->args = malloc(msg->num_args * sizeof(void *));
  msg->args[0] = node;
  enqueue(ctx->msg_queue, msg);
}

void node_set_msg_handler(Graph *node, int time, void **args) {}

void node_set_msg(UserCtx *ctx, Graph *node, t_nframes frame_time) {
  t_queue_msg *msg = malloc(sizeof(t_queue_msg));
  msg->msg = "kick node trig";
  msg->time = frame_time;
  msg->func = (MsgAction)node_set_msg_handler;
  int num_args = 1 + 2;

  msg->num_args = num_args;
  msg->args = malloc(num_args * sizeof(void *));
  msg->args[0] = node;
  msg->args[1] = "freq";
  t_sample freq = 220.0;
  /* msg->args[2] = freq; */
  enqueue(ctx->msg_queue, msg);
}
