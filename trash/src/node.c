#include "node.h"
#include "ctx.h"
#include "memory.h"
#include "signal.h"
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

Node *alloc_node(size_t obj_size, const char *name, size_t num_ins) {
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
  node->_block_offset = 0;

  node->ins = ALLOC_SIGS(num_ins);
  node->num_ins = num_ins;

  return node;
};

Node *make_node(size_t obj_size, node_perform perform, const char *name) {
  void *obj = allocate(obj_size);
  Node *node = allocate(sizeof(Node));
  node->perform = (node_perform *)perform;
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
  dest->ins[dest_sig_idx].size = source->out.size;
  dest->ins[dest_sig_idx].layout = source->out.layout;
}

// perform graph until you return tail of container's sub nodes - final node
// in the chain means use the output and additively write to an output
// channel
static node_perform container_perform(Node *node, int nframes,
                                      double seconds_per_frame) {
  if (node->_sub) {
    int block_offset = node->_block_offset;
    if (block_offset > 0) {
      node->_sub->_block_offset = block_offset;
    }

    Node *tail = (Node *)perform_graph(node->_sub, nframes, seconds_per_frame);
    Signal out = OUTS(tail);

    if (((container_node_data *)node->data)->write_to_output == true &&
        out.size == BUF_SIZE /*don't write control signals to DAC*/) {
      ctx_add_node_out_to_output(&out, nframes, seconds_per_frame);
    }
  }
}

static void update_container(Node *container, Node *head) {

  Node *chain_node = head;
  chain_node->parent = container;

  int num_ins = container->num_ins + chain_node->num_ins;
  while (chain_node->next) {
    chain_node = chain_node->next;

    num_ins += chain_node->num_ins;
    chain_node->parent = container;
  }

  container->out = chain_node->out;

  Signal *inputs = realloc(container->ins, sizeof(Signal) * num_ins);
  container->ins = inputs;
  container->num_ins = num_ins;

  for (chain_node = container->_sub; chain_node != NULL;
       chain_node = chain_node->next) {
    for (int i = 0; i < chain_node->num_ins; i++) {
      (inputs + i)->data = (chain_node->ins + i)->data;
      (inputs + i)->size = (chain_node->ins + i)->size;
      (inputs + i)->layout = (chain_node->ins + i)->layout;
    }
    inputs += chain_node->num_ins;
  }
}
/*
 * Node -> (ContainerNode
 *              Sub: Node
 *         )
 * */
Node *container_node(Node *sub) {
  Node *node = ALLOC_NODE(container_node_data, "Container", 0);
  ((container_node_data *)node->data)->write_to_output = true;
  node->perform = (node_perform)container_perform;
  node->_sub = sub;
  update_container(node, sub);

  return node;
}

Node *node_write_out(Node *node, int frame, double sample) {
  double mul = unwrap(node->mul, frame);

  double add = unwrap(node->add, frame);
  node->out.data[frame] = add + sample * mul;
  return node;
}

static node_perform lag_node_perform(Node *node, int nframes, double spf) {
  lag_node_data *data = node->data;

  double start = data->start;
  double target = data->target;
  double lagtime = data->lagtime;

  double step = (target - start) / (lagtime * spf);

  double sample = start;
  for (int f = 0; f < nframes; f++) {
    if (sample != target) {
      sample += step;
    }
    /* printf("lag samp %f\n", sample); */
    node_write_out(node, f, sample);
  }
}

static Node *lag_node(Signal *sig, double target_value, double lagtime) {
  Node *node = ALLOC_NODE(lag_node_data, "Lag", 1);
  if (sig->size != BUF_SIZE) {
    double init_value = sig->data[0];
    sig->data = realloc(sig->data, BUF_SIZE);
    for (int i = 0; i < BUF_SIZE; i++) {
      sig->data[i] = init_value;
    }
    sig->size = BUF_SIZE;
  }

  node->ins = sig;
  node->out = *sig;
  node->perform = (node_perform)lag_node_perform;

  lag_node_data *data = node->data;
  data->target = target_value;
  data->lagtime = lagtime;
  data->start = sig->data[sig->size - 1];

  return node;
}

Node *node_set_sig_double(Node *node, int sig_idx, double value) {

  if (sig_idx >= node->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return node;
  }
  Signal signal = IN(node, sig_idx);
  set_signal(signal, value);
  return node;
};

Node *node_set_sig_double_lag(Node *node, int sig_idx, double value,
                              double lagtime) {

  if (sig_idx >= node->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return node;
  }

  Node *lag = lag_node(node->ins + sig_idx, value, lagtime);
  Node *prev = node->prev;
  if (prev) {
    prev->next = lag;
    lag->prev = prev;
  } else {
    node->parent->_sub = lag;
  }
  lag->next = node;
  node->prev = lag;
  return node;
};

Node *node_set_sig_node(Node *node, int sig_idx, Node *src) {

  if (sig_idx >= node->num_ins) {
    printf("node has no input %d\n", sig_idx);
    return node;
  }

  Signal input = IN(node, sig_idx);

  input.data = src->out.data;
  input.size = src->out.size;

  return node;
}
Node *node_set_add_node(Node *node, Node *src) {
  node->add.data = src->out.data;
  node->add.size = src->out.size;
  return node;
}

Node *node_set_add_double(Node *node, double val) {
  set_signal(node->add, val);
  return node;
}

Node *node_set_mul_node(Node *node, Node *src) {
  node->mul.data = src->out.data;
  node->mul.size = src->out.size;
  return node;
}

Node *node_set_mul_double(Node *node, double val) {
  set_signal(node->mul, val);
  return node;
}

void node_build_ins(Node *node, int num_ins, double *init_values) {
  INS(node) = ALLOC_SIGS(num_ins);
  for (int i = 0; i < num_ins; i++) {
    init_signal(INS(node) + i, BUF_SIZE, *(init_values + i));
  }
  init_out_signal(&OUTS(node), BUF_SIZE, 1);
}

int get_block_offset(Node *node) {
  int offset = node->_block_offset;
  node->_block_offset = 0;
  return offset;
}
