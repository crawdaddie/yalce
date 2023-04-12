#include "node.h"
#include "audio/signal.h"
#include "ctx.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

/* typedef struct { */
/*   Node node_pool[512]; */
/*   Node *free_node; */
/* } node_pool; */

/* node_pool NodePool; */

node_perform perform_graph(Node *head, int nframes, double seconds_per_frame) {
  if (head->killed) {
    Node *next = head->next;
    /* ctx_remove_node(head); */
    if (next) {
      return perform_graph(next, nframes, seconds_per_frame);
    }
  }

  if (!head) {
    return NULL;
  };

  if (head->perform && !(head->killed)) {
    head->perform(head, nframes, seconds_per_frame);
  }

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
  /* Node *node = NodePool.free_node; */
  /* NodePool.free_node++; */
  node->add.data = malloc(sizeof(double));
  *node->add.data = 0.0;
  node->add.size = 1;
  node->mul.data = malloc(sizeof(double));
  *node->mul.data = 1.0;
  node->mul.size = 1;
  /* node->mul = new_signal_heap_default(1, 1, 1.0); */

  node->data = obj;
  node->name = name;
  node->killed = false;
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

Node *node_add_after(Node *before, Node *after) {
  Node *next = before->next;
  before->next = after;
  after->next = next;
  return after;
}

Node *chain_nodes(Node *source, Node *dest, int dest_sig_idx) {
  node_add_after(source, dest);
  dest->ins[dest_sig_idx].data = source->out.data;
}

static node_perform perform_container(Node *node, int nframes,
                                      double seconds_per_frame) {
  if (node->_sub) {
    Node *tail = (Node *)perform_graph(node->_sub, nframes, seconds_per_frame);
    // perform graph until you return tail of container's sub nodes - final node
    // in the chain means use the output and additively write to an output
    // channel
    Signal out = OUTS(tail);

    if (((container_node_data *)node->data)->write_to_output == true &&
        out.size == BUF_SIZE /*don't write control signals to DAC*/) {
      ctx_add_node_out_to_output(&out, nframes, seconds_per_frame);
    }
  }
}

Node *container_node(Node *sub) {
  Node *node = ALLOC_NODE(container_node_data, "Container");
  ((container_node_data *)node->data)->write_to_output = true;
  node->perform = perform_container;
  node->_sub = sub;
  Node *chain_node = sub;
  int num_ins = chain_node->num_ins;
  while (chain_node->next) {
    num_ins += chain_node->num_ins;
    chain_node = chain_node->next;
  }
  /* Signal *inputs = malloc(sizeof(Signal) * num_ins); */
  node->out = chain_node->out;

  /* for (chain_node = node->_sub; chain_node != NULL; */
  /*      chain_node = chain_node->next) { */
  /* } */
  /*
    node->num_ins = num_ins;
    node->ins = inputs;

    for (chain_node = node->_sub; chain_node != NULL;
         chain_node = chain_node->next) {
      chain_node->ins = inputs;
      for (int i = 0; i < chain_node->num_ins; i++) {
        (inputs + i)->data = (chain_node->ins + i)->data;
        (inputs + i)->size = (chain_node->ins + i)->size;
        (inputs + i)->layout = (chain_node->ins + i)->layout;
      }
      inputs += chain_node->num_ins;
    }
    */

  return node;
}

Node *node_write_out(Node *node, int frame, double sample) {
  double mul = unwrap(node->mul, frame);

  double add = unwrap(node->add, frame);
  node->out.data[frame] = add + sample * mul;
  return node;
}
