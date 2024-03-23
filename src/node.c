#include "node.h"
#include "common.h"
#include "ctx.h"
#include "signal.h"
#include <stdio.h>
#include <stdlib.h>

// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
  node->ins = &ins;
  node->num_ins = 1;
  node->out = out;
  node->perform = (node_perform)perform;
  return node;
}

Node *pipe_output(Node *send, Node *recv) {
  recv->ins = &send->out;
  return recv;
}

Node *pipe_sig(Signal *send, Node *recv) {
  recv->ins = &send;
  return recv;
}
Node *pipe_output_to_idx(int idx, Node *send, Node *recv) {
  recv->ins[idx] = send->out;
  return recv;
}

Node *pipe_sig_to_idx(int idx, Signal *send, Node *recv) {
  recv->ins[idx] = send;
  return recv;
}

Node *add_to_dac(Node *node) {
  node->type = OUTPUT;
  return node;
}
Node *chain_set_out(Node *chain, Node *out) {
  chain->out = out->out;
  return chain;
}

static node_perform group_perform(Node *group, int nframes, double spf) {
  group_state *state = group->state;
  perform_graph(state->graph->head, nframes, spf, group->out, 0);
}
Node *group_new() {
  group_state *state = malloc(sizeof(group_state));
  return node_new(state, (node_perform *)group_perform, &(Signal){},
                  get_sig(1));
}

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, get_sig(1)); }

Node *chain_with_inputs(int num_ins, double *defaults) {
  Node *chain = node_new(NULL, NULL, NULL, &(Signal){});

  chain->ins = malloc(num_ins * (sizeof(Signal *)));
  chain->num_ins = num_ins;
  for (int i = 0; i < num_ins; i++) {
    chain->ins[i] = get_sig_default(1, defaults[i]);
  }
  return chain;
}

Node *node_set_input_signal(Node *node, int num_in, Signal *sig) {
  node->ins[num_in] = sig;
  return node;
}

void node_add_after(Node *tail, Node *node) {
  if (tail == NULL) {
    printf("Previous node cannot be NULL\n");
    return;
  }
  node->next = tail->next;
  if (tail->next != NULL) {
    tail->next->prev = node;
  }
  tail->next = node;
  node->prev = tail;
};

// Function to add a node before a particular node
void node_add_before(Node *head, Node *node) {
  if (head == NULL) {
    printf("Next node cannot be NULL\n");
    return;
  }
  node->prev = head->prev;
  if (head->prev != NULL) {
    head->prev->next = node;
  }
  head->prev = node;
  node->next = head;
}

void graph_add_tail(Graph *graph, Node *node) {
  if (graph->tail == NULL) {
    graph->tail = node;
    return;
  }
  node_add_after(graph->tail, node);
  graph->tail = node;
  if (graph->head == NULL) {
    graph->head = graph->tail;
  }
};

void graph_add_head(Graph *graph, Node *node) {
  if (graph->head == NULL) {
    graph->head = node;
    return;
  }
  node_add_before(graph->head, node);
  graph->head = node;

  if (graph->tail == NULL) {
    graph->tail = graph->head;
  }
};

void graph_delete_node(Graph *graph, Node *node) {

  if (graph == NULL || node == NULL) {
    printf("Invalid arguments\n");
    return;
  }
  if (graph->head == node) {
    graph->head = node->next;
  }
  if (graph->tail == node) {
    graph->tail = node->prev;
  }
  if (node->prev != NULL) {
    node->prev->next = node->next;
  }
  if (node->next != NULL) {
    node->next->prev = node->prev;
  }

  free(node);
};

// Function to print the doubly linked list
void graph_print(Graph *dll) {
  if (dll == NULL || dll->head == NULL) {
    printf("Doubly linked list is empty\n");
    return;
  }
  Node *temp = dll->head;
  while (temp != NULL) {
    printf("[ %p [%s] [%s] ", temp, temp->name, node_type_names[temp->type]);
    printf("ins: ");
    double *buf_pool_start = get_buf_pool_start();
    for (int i = 0; i < temp->num_ins; i++) {
      double *buf = temp->ins[i]->buf;
      int buf_offset = (buf - buf_pool_start) / BUF_SIZE;
      printf("(\x1b[38;5;%dmbuf %p\x1b[0m [%d]), ", buf, buf,
             temp->ins[i]->layout);
    }

    double *out_buf = temp->out->buf;
    printf("out: \x1b[38;5;%dmbuf %p\x1b[0m [%d]", out_buf, out_buf,
           temp->out->layout);

    printf(" ]");
    temp = temp->next;
    if (temp) {
      printf("\n");
    }
  }
  printf("\n");
}
