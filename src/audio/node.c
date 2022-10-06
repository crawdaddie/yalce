#include "node.h"
#include <stdlib.h>

void free_data(NodeData *data) { free(data); }
void free_node(Node *node) {
  NodeData *data = node->data;
  free_data(data);
  free(node);
}
double *get_buffer() { return calloc(2048, sizeof(double)); }

Node *alloc_node(NodeData *data, double *in, t_perform perform, char *name,
                 t_free_node custom_free_node) {
  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = name ? (name) : "";
  node->perform = perform;
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

int delay_til_schedule_time(double schedule, int frame, double seconds_offset,
                            double seconds_per_frame) {
  if (schedule == 0.0) {
    return 0;
  };
  double cur_time = seconds_offset + frame * seconds_per_frame;
  if (schedule < cur_time) {
    return 1;
  }
  return 0;
}

void perform_null(Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset) {
  double *out = node->out;

  for (int i = 0; i < frame_count; i++) {
    schedule();
    out[i] = 0.0;
  }
}

void perform_node_mul(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset) {
  double *in = node->in;
  double *out = node->out;
  double *mul = node->mul;
  for (int i = 0; i < frame_count; i++) {
    schedule();
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
                      double seconds_offset) {
  double *in = node->in;
  double *out = node->out;
  double *add = node->add;
  for (int i = 0; i < frame_count; i++) {
    schedule();
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
