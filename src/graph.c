#include "graph.h"
#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>

static node_perform group_perform(Node *group, int nframes, double spf) {
  Graph *graph = group->state;
  perform_graph(graph->head, nframes, spf, group->out, 0);
}
static char *group_name = "group";
Node *group_new(int chans) {
  Graph *graph = malloc(sizeof(Graph));
  Node *g = node_new((void *)graph, (node_perform *)group_perform, &(Signal){},
                     &(Signal){});
  g->num_ins = 0;
  // g->ins = NULL;
  g->is_group = true;
  g->name = group_name;
  g->out = get_sig(chans);
  return g;
}

Node *group_with_inputs(int num_ins, double *defaults) {
  Node *group = group_new(1);

  group->ins = malloc(num_ins * (sizeof(Signal *)));
  group->num_ins = num_ins;
  for (int i = 0; i < num_ins; i++) {
    group->ins[i] = get_sig_default(1, defaults[i]);
  }
  return group;
}

void group_add_tail(Node *group, Node *node) {
  graph_add_tail(group->state, node);
  node->parent = group;
}

void group_add_head(Node *group, Node *node) {
  graph_add_head(group->state, node);
  node->parent = group;
}

// Function to print the graph
//
#define COLORIZE(color_code, str) "\x1b[38;5;" #color_code "m" str "\x1b[0m"

void graph_print(Graph *graph, int indent) {

  if (graph == NULL || graph->head == NULL) {
    return;
  }

  Node *node = graph->head;
  while (node != NULL) {
    printf("%*s[ " COLORIZE(127, "%p") " [%s] [%s] ", indent, "", node,
           node->name, node_type_names[node->type]);
    printf("ins: ");
    double *buf_pool_start = get_buf_pool_start();
    for (int i = 0; i < node->num_ins; i++) {
      double *buf = node->ins[i]->buf;
      printf("(" COLORIZE(127, "buf %p") " [%d]), ", buf, node->ins[i]->layout);
    }

    double *out_buf = node->out->buf;
    printf("out: " COLORIZE(127, "buf %p") " [%d]", out_buf, node->out->layout);

    printf(" ]");

    if (node->is_group) {
      printf("\n");
      graph_print((Graph *)node->state, indent + 1);
    }
    node = node->next;
    if (node) {
      printf("\n");
    }
  }
  printf("\n");
}

Graph *graph_add_tail(Graph *graph, Node *node) {
  if (graph->tail == NULL && graph->head == NULL) {
    graph->tail = node;
    graph->head = node;
    return graph;
  }
  node_add_after(graph->tail, node);
  graph->tail = node;
  if (graph->head == NULL) {
    graph->head = graph->tail;
  }
  return graph;
};

Graph *graph_add_head(Graph *graph, Node *node) {
  if (graph->head == NULL) {
    graph->head = node;
    return graph;
  }
  node_add_before(graph->head, node);
  graph->head = node;

  if (graph->tail == NULL) {
    graph->tail = graph->head;
  }
  return graph;
};

Graph *graph_delete_node(Graph *graph, Node *node) {

  if (graph == NULL || node == NULL) {
    printf("Invalid arguments\n");
    return graph;
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

  free_node(node);
  return graph;
};
