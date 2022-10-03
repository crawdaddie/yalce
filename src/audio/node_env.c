#include "node.h"
typedef struct {
  double attack;
  double sustain;
  double release;
  double offset;
} env_data;

void ramp_value_tick(double *val, double target, double speed) {}

void perform_env(Node *node, int frame_count, double seconds_per_frame,
                 double seconds_offset) {
  env_data *data = (env_data *)node->data;
  double level = 0.0;
  double env_offset = data->offset;
  double attack = data->attack;
  double sustain = data->sustain;
  double release = data->release;
  double *out = node->out;

  for (int i = 0; i < frame_count; i++) {
    double time_ms = (seconds_offset + i * seconds_per_frame) * 1000;
    double input = out[i];
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
    };
    out[i] = input * level;
  }
}

Node *get_env_node(double attack, double sustain, double release,
                   double offset) {
  env_data *data = malloc(sizeof(env_data));
  data->attack = attack;
  data->sustain = sustain;
  data->release = release;
  data->offset = offset;
  return alloc_node((NodeData *)data, NULL, (t_perform)perform_env, "env",
                    NULL);
}

void reset_env(Node *node, double offset) {
  env_data *data = (env_data *)node->data;
  data->offset = offset;
}
