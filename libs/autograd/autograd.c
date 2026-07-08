#include "ylc_datatypes.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int32_t size;
  void *data;
} arr;
YLC_ARRAY_TYPE(int32_t);

typedef struct Tensor {
  _YLC_int32_t_Array shape;
  _DoubleArray data;
  _DoubleArray grad;
  void *forward;
  void *backward;
  struct {
    int32_t size;
    struct Tensor *data;
  } operands;
  int32_t op;
  int32_t id;
} Tensor;

typedef struct {
  int32_t local_id;
  int32_t tensor_id;
  int32_t op;
  int32_t operand_start;
  int32_t operand_count;
} GraphNode;

typedef struct {
  int32_t size;
  int32_t capacity;
  GraphNode *nodes;
  int32_t edge_size;
  int32_t edge_capacity;
  int32_t *edges;
} Graph;

static const char *op_name(int32_t op) {
  switch (op) {
  case 0:
    return "Leaf";
  case 1:
    return "Add";
  case 2:
    return "Sub";
  case 3:
    return "Mul";
  case 4:
    return "Sq";
  case 5:
    return "Neg";
  case 6:
    return "Div";
  case 7:
    return "Relu";
  case 8:
    return "Sum";
  case 9:
    return "Matmul";
  default:
    return "Unknown";
  }
}

static int graph_find_tensor_id(Graph *graph, int32_t tensor_id) {
  for (int i = 0; i < graph->size; i++) {
    if (graph->nodes[i].tensor_id == tensor_id) {
      return i;
    }
  }
  return -1;
}

static void graph_reserve(Graph *graph, int32_t needed) {
  if (graph->capacity >= needed) {
    return;
  }

  int32_t next_capacity = graph->capacity ? graph->capacity * 2 : 32;
  while (next_capacity < needed) {
    next_capacity *= 2;
  }

  graph->nodes = realloc(graph->nodes, sizeof(GraphNode) * next_capacity);
  graph->capacity = next_capacity;
}

static int32_t graph_append_edge(Graph *graph, int32_t node_id) {
  if (graph->edge_capacity <= graph->edge_size) {
    int32_t next_capacity =
        graph->edge_capacity ? graph->edge_capacity * 2 : 64;
    graph->edges = realloc(graph->edges, sizeof(int32_t) * next_capacity);
    graph->edge_capacity = next_capacity;
  }

  int32_t edge_id = graph->edge_size++;
  graph->edges[edge_id] = node_id;
  return edge_id;
}

static int graph_collect(Graph *graph, Tensor *tensor) {
  int existing = graph_find_tensor_id(graph, tensor->id);
  if (existing >= 0) {
    return existing;
  }

  int32_t operand_count = tensor->operands.size;
  int32_t *operand_ids = NULL;
  if (operand_count > 0) {
    operand_ids = malloc(sizeof(int32_t) * operand_count);
  }

  for (int i = 0; i < operand_count; i++) {
    operand_ids[i] = graph_collect(graph, tensor->operands.data + i);
  }

  int32_t operand_start = graph->edge_size;
  for (int i = 0; i < operand_count; i++) {
    graph_append_edge(graph, operand_ids[i]);
  }
  free(operand_ids);

  graph_reserve(graph, graph->size + 1);
  int32_t local_id = graph->size++;
  graph->nodes[local_id] = (GraphNode){
      .local_id = local_id,
      .tensor_id = tensor->id,
      .op = tensor->op,
      .operand_start = operand_start,
      .operand_count = operand_count,
  };
  return local_id;
}

static void graph_free(Graph *graph) {
  free(graph->nodes);
  free(graph->edges);
}

static void graph_print(Graph *graph, int output_id) {
  printf("graph nodes: %d edges: %d output: %%%d\n", graph->size,
         graph->edge_size, output_id);
  for (int i = 0; i < graph->size; i++) {
    GraphNode *node = graph->nodes + i;
    printf("%%%d tensor_id:%d %s", node->local_id, node->tensor_id,
           op_name(node->op));
    if (node->operand_count > 0) {
      printf(" operands:");
      for (int j = 0; j < node->operand_count; j++) {
        printf(" %%%d", graph->edges[node->operand_start + j]);
      }
    }
    printf("\n");
  }
}

void autograd_proc_graph(Tensor *g) {
  Graph graph = {0};
  int output_id = graph_collect(&graph, g);
  graph_print(&graph, output_id);
  fflush(stdout);
  graph_free(&graph);
}
