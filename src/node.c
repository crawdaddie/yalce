#include "node.h"
#include "memory.h"
#include <string.h>

node_perform perform_graph(struct Node *head, int nframes,
                           double seconds_per_frame) {
  if (!head) {
    return NULL;
  };

  if (head->perform) {
    head->perform(head, nframes, seconds_per_frame);
  }
  /*

  if (head->_graph) {
    perform_graph(head->_graph, nframes, seconds_per_frame);
  };
  */

  Node *next = head->next;
  if (next) {
    return perform_graph(next, nframes,
                         seconds_per_frame); // keep going until you return tail
  };
  return head;
}

Node *alloc_node(size_t obj_size, const char *name) {
  void *obj = allocate(obj_size);
  Node *node = allocate(sizeof(Node));
  node->data = obj;
  node->name = name;
  return node;
};

Node *make_node(size_t obj_size, node_perform perform, const char *name) {
  void *obj = allocate(obj_size);
  Node *node = allocate(sizeof(Node));
  node->perform = perform;
  node->data = obj;
  node->name = name;
  return node;
};
