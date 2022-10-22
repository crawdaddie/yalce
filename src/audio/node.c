#include "node.h"
#include "util.c"
#include <stdlib.h>

void free_data(NodeData *data) { free(data); }
void free_node(Node *node) {
  NodeData *data = node->data;
  free_data(data);
  free(node->out);
  free(node);
}
double *get_buffer() { return calloc(2048, sizeof(double)); }
void perform_null(Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset, double schedule) {
  double *out = node->out;
  for (int i = 0; i < frame_count; i++) {
    sched();
    out[i] = 0.0;
  };
}

Node *alloc_node(NodeData *data, double *in, t_perform perform, char *name,
                 t_free_node custom_free_node) {
  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = name ? (name) : "";
  node->perform = perform ? (t_perform)perform : (t_perform)perform_null;
  node->next = NULL;
  node->mul = NULL;
  node->add = NULL;
  node->data = data;
  node->in = in;
  node->out = get_buffer();
  node->free_node = (custom_free_node) ? free_node : custom_free_node;
  node->should_free = 0;
  node->schedule = 0.0;
  return node;
}

double get_sample_interp(double read_ptr, double *buf, int max_frames) {
  double r = read_ptr;
  if (r >= max_frames) {
    return 0.0;
  }
  if (r <= 0) {
    r = max_frames + r;
  }
  int frame = ((int)r);
  double fraction = r - frame;
  double result = buf[frame] * fraction;
  result += buf[frame + 1] * (1.0 - fraction);
  return result;
}

void set_ctrl_val(CtrlVal *ctrl, int i, double val) {
  if (ctrl->size == 1) {
    ctrl->val = &val;
  }
  ctrl->val[i] = val;
};

void map_ctrl_to_buf(CtrlVal *ctrl, double *buf, int size) {
  ctrl->val = buf;
  ctrl->size = size;
}

double get_ctrl_val(CtrlVal *ctrl, double i) {
  if (ctrl->size == 1) {
    return *ctrl->val;
  };
  return get_sample_interp(i, ctrl->val, ctrl->size);
}

int set_pointer_val(double *ptr, int i, double val) {
  if (ptr == NULL) {
    return 1;
  }
  return 0;
}

int delay_til_schedule_time(double schedule, int frame, double seconds_offset,
                            double seconds_per_frame) {
  if (schedule == 0.0) {
    return 0;
  };
  double cur_time = seconds_offset + frame * seconds_per_frame;
  if (schedule > cur_time) {
    return 1;
  };
  return 0;
}

void perform_node_mul(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset, double schedule) {
  double *in = node->in;
  double *out = node->out;
  double *mul = node->mul;
  for (int i = 0; i < frame_count; i++) {
    if (mul == NULL) {
      return;
    }
    sched();
    out[i] = in[i] * mul[i];
  }
}

Node *node_mul(Node *node_a, Node *node_b) {
  Node *node = alloc_node(NULL, NULL, (t_perform)perform_node_mul, "mul", NULL);
  node->in = node_a->out;
  node->mul = node_b->out;
  node_b->next = node_a;
  node_a->next = node;
  return node;
}
void perform_node_add(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset, double schedule) {
  double *in = node->in;
  double *out = node->out;
  double *add = node->add;
  for (int i = 0; i < frame_count; i++) {
    sched();
    out[i] = in[i] + add[i];
  }
}

Node *node_add(Node *node_a, Node *node_b) {
  Node *node = alloc_node(NULL, NULL, (t_perform)perform_node_add, "add", NULL);
  node->in = node_a->out;
  node->add = node_b->out;
  node_b->next = node_a;
  node_a->next = node;
  return node;
}

Node *node_add_to_tail(Node *node, Node *prev) {
  prev->next = node;
  return node; // returns tail
};

void debug_node(Node *node, char *text) {
  if (text)
    printf("%s\n", text);
  printf("node name: %s\n", node->name);
  printf("\tnode schedule %f\n", node->schedule);
  printf("\tnode &: %#08x\n", node);
  printf("\tnode out &: %#08x\n", node->out);
  printf("\tnode in &: %#08x\n", node->in);
  printf("\tnode perform: %#08x\n", node->perform);
  printf("\tnode next: %#08x\n", node->next);
  /* printf("node mul: %#08x\n", node->mul); */
  printf("\tnode size: %d\n", sizeof(*node));
  printf("\tnode finished: %d\n", node->should_free);
  /* printf("node add: %#08x\n", node->add); */
}
