#include "./audio_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AudioGraph *_graph = NULL;

double *allocate_buffer_from_pool(AudioGraph *graph, int size) {
  if (!graph) {
    return malloc(sizeof(double) * size);
  }

  // Ensure we have enough space
  if (graph->buffer_pool_size + size > graph->buffer_pool_capacity) {
    printf("realloc buffer pool?? %d %d\n", graph->buffer_pool_size + size,
           graph->buffer_pool_capacity);
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

Node *allocate_node_in_graph(AudioGraph *graph, int state_size) {
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
  char *state = (char *)graph->nodes_state_memory + node->state_offset;
  return state;
}
void perform_audio_graph(Node *_node, AudioGraph *graph, Node *_inputs[],
                         int nframes, double spf) {

  // int frame_offset = _node->frame_offset;
  // printf("perform audio graph %p\n", _node);
  int node_count = graph->node_count;
  Node *node = graph->nodes;
  Node *inputs[MAX_INPUTS];

  while (node_count--) {
    // node->frame_offset = frame_offset;
    // offset_node_bufs(node, frame_offset);

    if (node->perform) {
      __node_get_inputs(node, graph, inputs);
      char *state = __node_get_state(node, graph);
      node->perform(node, state, inputs, nframes, spf);
      // printf("node trig end %d\n", node->trig_end);

      if (node->trig_end == true) {
        _node->trig_end = true;
      }
    }
    // unoffset_node_bufs(node, frame_offset);

    node++;
  }
}
