#include "audio/graph.h"
#include "config.h"
#include <stdlib.h>

typedef struct kick_data {
  sample_t *out;
  sample_t ramp;
} kick_data;

t_perform perform_kick(Graph *graph, nframes_t nframes) {
  kick_data *data = (kick_data *)graph->data;

  double radians_per_second = 220 * 2.0 * PI;
  for (int i = 0; i < nframes; i++) {

    sample_t sample = sin(data->ramp * radians_per_second);
    graph->out[i] = sample * (data->ramp <= 0.05) * 0.25;
    data->ramp = fmod(data->ramp + (sample_t)1.0 / 48000, 1.0);
  }
}

typedef struct action_node_data {
  Graph *target;
} action_node_data;

void perform_trigger_action(Graph *action_node, nframes_t nframes) {
  action_node_data *data = (action_node_data *)action_node->data;

  Graph *target = data->target;

  for (int i = 0; i < nframes; i++) {
    if (i < action_node->schedule) {
      break;
    }
    if (i == action_node->schedule) {
      debug_node(target, "resetting node");
      ((kick_data *)((action_node_data *)action_node->data)->target->data)
          ->ramp = 0.0;
    } else {
      action_node->should_free = 1;
      break;
    };
  }
}

void trigger_kick(Graph *graph, int time, void *ref) {
  Graph *kick_node = (Graph *)ref;
  Graph *action_node = malloc(sizeof(Graph) + sizeof(action_node_data));
  action_node_data *data = malloc(sizeof(action_node_data));
  data->target = ref;
  action_node->data = data;
  add_before(ref, action_node);
}

void make_kick(Graph *graph, int time, void *ref) {
  kick_data *data = malloc(sizeof(kick_data));
  data->out = calloc(256, sizeof(sample_t));
  data->ramp = 0.0;
  sample_t *out = calloc(256, sizeof(sample_t));
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
