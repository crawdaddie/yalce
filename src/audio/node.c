#include "node.h"
#include <stdlib.h>

void free_data(NodeData *data) { free(data); }
void free_node(Node *node) {
  NodeData *data = node->data;
  free_data(data);
  free(node->out);
  free(node);
}
double *get_buffer() { return malloc(sizeof(double) * node_frame_size); }

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
  node->free_node = custom_free_node == NULL ? free_node : custom_free_node;
  return node;
}

void perform_node_mul(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset) {
  double *in = node->in;
  double *out = node->out;
  double *mul = node->mul;
  for (int i = 0; i < frame_count; i++) {
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
  return node;
};

void debug_node(Node *node, char *text) {
  if (text)
    printf("%s\n", text);
  printf("node name: %s\n", node->name);
  printf("node &: %#08x\n", node);
  printf("node out &: %#08x\n", node->out);
  printf("node in &: %#08x\n", node->in);
  printf("node perform: %#08x\n", node->perform);
  printf("node next: %#08x\n", node->next);
  printf("node mul: %#08x\n", node->mul);
  /* printf("node add: %#08x\n", node->add); */
  printf("-------\n");
}
