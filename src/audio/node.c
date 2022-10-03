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
  node->data = data;
  node->in = in;
  node->out = get_buffer();
  node->free_node = custom_free_node == NULL ? free_node : custom_free_node;
  return node;
}

void debug_node(Node *node, char *text) {
  if (text)
    printf("%s\n", text);
  printf("node name: %s\n", node->name);
  printf("node &: %#08x\n", node);
  printf("node out &: %#08x\n", node->out);
  printf("node in &: %#08x\n", node->in);
  printf("node perform: %#08x\n", node->perform);
  printf("node next: %#08x\n", node->next);
  printf("-------\n");
}
