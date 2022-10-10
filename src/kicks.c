#include "audio/graph.h"
#include "config.h"
#include <stdlib.h>

typedef void (*t_trigger)(void *data, nframes_t i);
typedef struct kick_data {
  sample_t *out;
  sample_t ramp;
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

t_perform perform_kick(Graph *graph, nframes_t nframes) {
  kick_data *data = (kick_data *)graph->data;

  double radians_per_second = 220 * 2.0 * PI;
  for (int i = 0; i < nframes; i++) {
    process_triggers(data, i);
    sample_t sample = sin(data->ramp * radians_per_second);
    graph->out[i] = sample * (data->ramp <= 0.05) * 0.25;
    data->ramp += (sample_t)1.0 / 48000;
  }
}

void set_kick_trigger(Graph *kick_node, int time, Graph *kick_node_ref) {
  kick_data *data = (kick_data *)kick_node_ref->data;
  data->trigger_frame = time;
}

void make_kick(Graph *graph, int time, void *ref) {
  kick_data *data = malloc(sizeof(kick_data));
  data->out = calloc(256, sizeof(sample_t));
  data->ramp = 0.0;
  data->trigger_frame = -1;
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
