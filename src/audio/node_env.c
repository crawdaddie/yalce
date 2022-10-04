#include "node.h"
typedef struct {
  double attack;
  double sustain;
  double release;
  double offset;
  Node *ctx;
  void (*on_free)(Node *ctx)
} env_data;

void ramp_value_tick(double *val, double target, double speed) {}

void perform_env(Node *node, int frame_count, double seconds_per_frame,
                 double seconds_offset) {
  env_data *data = (env_data *)node->data;
  double level = 0.0;
  double *out = node->out;
  double attack = data->attack;
  double sustain = data->sustain;
  double release = data->release;

  for (int i = 0; i < frame_count; i++) {
    double env_offset = data->offset;
    double time_ms = (seconds_offset + i * seconds_per_frame) * 1000;
    if (time_ms <= env_offset + attack) {
      level = (time_ms - env_offset) / attack;
    } else if ((time_ms > env_offset + attack) &&
               (time_ms <= env_offset + attack + sustain)) {
      level = 1.0;
    } else if ((time_ms > env_offset + attack + sustain) &&
               (time_ms <= env_offset + attack + sustain + release)) {
      level = (attack + sustain + release - (time_ms - env_offset)) / release;
    } else {
      level = 0.0;
      if ((time_ms > env_offset + attack + sustain + release)) {
        if (data->on_free)
          data->on_free(data->ctx);
      }
    };
    out[i] = level;
  }
}

Node *get_env_node(double attack, double sustain, double release,
                   double offset) {
  env_data *data = malloc(sizeof(env_data) + sizeof(Node));
  data->attack = attack;
  data->sustain = sustain;
  data->release = release;
  data->offset = offset;
  data->ctx = NULL;
  data->on_free = NULL;
  return alloc_node((NodeData *)data, NULL, NULL, (t_perform)perform_env, "env",
                    NULL);
}

void env_set_on_free(Node *env_node, Node *ctx, void (*on_free)(Node *ctx)) {
  ((env_data *)env_node->data)->ctx = ctx;
  ((env_data *)env_node->data)->on_free = on_free;
}

void reset_env(Node *node, double offset) {
  env_data *data = (env_data *)node->data;
  data->offset = offset;
}
