#include "./audio_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AudioGraph *_graph = NULL;

double *allocate_buffer_from_pool(AudioGraph *graph, int size) {
  if (!graph) {
    return calloc(size, sizeof(double));
  }

  // Ensure we have enough space
  if (graph->buffer_pool_size + size > graph->buffer_pool_capacity) {
    // printf("realloc buffer pool?? %d %d\n", graph->buffer_pool_size + size,
    //        graph->buffer_pool_capacity);
    graph->buffer_pool_capacity *= 2;
    graph->buffer_pool =
        realloc(graph->buffer_pool, graph->buffer_pool_capacity);

    // Realloc buffer pool if needed
    // ...
  }

  double *buffer = &graph->buffer_pool[graph->buffer_pool_size];
  graph->buffer_pool_size += size;
  // while (size--) {
  //   *(buffer + size) = 0.0;
  // }
  return buffer;
}

int state_offset_ptr_in_graph(AudioGraph *graph, int size) {
  if (!graph) {
    return 0;
  }

  size = (size + 7) & ~7; // 8-byte alignment

  int offset = graph->state_memory_size;
  if (graph->state_memory_size + size > graph->state_memory_capacity) {
    graph->state_memory_capacity *= 2;
    graph->nodes_state_memory =
        realloc(graph->nodes_state_memory, graph->state_memory_capacity);
  }

  graph->state_memory_size += size;

  return offset;
}

char *state_ptr(AudioGraph *graph, NodeRef node) {
  if (!graph) {
    return (char *)((Node *)node + 1);
  }
  return graph->nodes_state_memory + node->state_offset;
}

Node *__allocate_node_in_graph(AudioGraph *graph, int state_size) {
  if (!graph) {
    return malloc(sizeof(Node) + state_size);
  }

  int idx = graph->node_count++;

  if (graph->node_count >= graph->capacity) {
    graph->capacity *= 2;
    graph->nodes = realloc(graph->nodes, graph->capacity * sizeof(Node));
  }

  Node *node = &graph->nodes[idx];
  node->node_index = idx;
  return node;
}

Node *allocate_node_in_graph(AudioGraph *graph, int state_size) {
  if (!graph) {
    return malloc(sizeof(Node) + state_size);
  }

  int idx = graph->node_count++;

  // Resize nodes array if needed
  if (graph->node_count >= graph->capacity) {
    graph->capacity *= 2;
    graph->nodes = realloc(graph->nodes, graph->capacity * sizeof(Node));
  }

  // Check if we need more state memory and reallocate if necessary
  if (graph->state_memory_size + state_size > graph->state_memory_capacity) {
    // Double the state memory capacity or increase by what we need, whichever
    // is larger
    int new_capacity = graph->state_memory_capacity * 2;
    if (graph->state_memory_size + state_size > new_capacity) {
      new_capacity =
          graph->state_memory_size + state_size + 1024; // Add extra padding
    }

    // Reallocate the state memory
    graph->nodes_state_memory =
        realloc(graph->nodes_state_memory, new_capacity);
    graph->state_memory_capacity = new_capacity;
  }

  // Initialize node and return
  Node *node = &graph->nodes[idx];
  node->node_index = idx;

  return node;
}

void print_node(NodeRef n) {
  printf("node (%s) ", n->meta);

  if (strcmp(n->meta, "const") == 0) {
    printf("%f", n->output.buf[0]);
  }
  printf("\n\t[");

  for (int j = 0; j < n->num_inputs; j++) {
    printf("%d, ", n->connections[j].source_node_index);
  }
  printf("]\n");
}

void print_graph(AudioGraph *g) {
  int node_count = g->node_count;

  for (int i = 0; i < node_count; i++) {
    Node *n = g->nodes + i;

    printf("[%d] node (%s) ", i, n->meta);

    if (strcmp(n->meta, "const") == 0) {
      printf("%f", n->output.buf[0]);
    }
    printf("\n\t[");

    for (int j = 0; j < n->num_inputs; j++) {
      printf("%d, ", n->connections[j].source_node_index);
    }
    printf("]\n");
  }
  printf("buffer pool: %d\n", g->buffer_pool_size);
  printf("state size: %d\n", g->state_memory_size);
}

void __node_get_inputs(Node *node, AudioGraph *graph, Node *inputs[]) {
  int num_inputs = node->num_inputs;
  for (int i = 0; i < num_inputs; i++) {
    inputs[i] = graph->nodes + node->connections[i].source_node_index;
  }
}

char *__node_get_state(Node *node, AudioGraph *graph) {
  if (graph == NULL) {
    // no graph context, node + state probably allocated together
    return (char *)node + sizeof(Node);
  }
  if (node->state_ptr) {
    return node->state_ptr;
  }
  char *state = (char *)graph->nodes_state_memory + node->state_offset;
  return state;
}
void perform_audio_graph(Node *_node, AudioGraph *graph, Node *_inputs[],
                         int nframes, double spf) {

  int node_count = graph->node_count;
  Node *node = graph->nodes;
  Node *inputs[MAX_INPUTS];

  while (node_count--) {

    if (node->perform) {
      __node_get_inputs(node, graph, inputs);
      char *state = __node_get_state(node, graph);
      node->perform(node, state, inputs, nframes, spf);

      // if (node->node_math) {
      //   for (int i = 0; i < node->output.size * node->output.layout; i++) {
      //     node->output.buf[i] = node->node_math(node->output.buf[i]);
      //   }
      // }
      //
      if (node->trig_end == true) {
        _node->trig_end = true;
        memset(_node->output.buf, 0,
               _node->output.size * _node->output.layout * sizeof(double));
      }
    }

    node++;
  }
}
