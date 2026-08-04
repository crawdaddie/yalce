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

/* ---- C-level allocation pool for intermediate Double arrays ----
 *
 * A simple bump allocator: one big malloc'd buffer, a cursor that advances
 * on each alloc, and a reset that rewinds the cursor. No per-element
 * malloc/free, no RC — the YLC side treats the returned _DoubleArray as a
 * raw borrow (not RC-managed), so perceus won't try to dup/drop it.
 *
 * Usage from YLC:
 *   let pool_alloc = extern fn Int -> Array of Double;   // get a zeroed array
 *   let pool_reset = extern fn () -> ();                  // rewind for next pass
 *
 * Call pool_reset() at the start of each training step. The arrays returned
 * by pool_alloc are valid until the next pool_reset().
 */

#define POOL_CAPACITY (1 << 20)  /* 1 MiB of doubles */
static double *pool_base = NULL;
static int32_t pool_cursor = 0;

/* Each allocation reserves 8 bytes (YlcRcHeader {int32 rc=0, int32 tag=0})
 * before the payload so perceus RC drop/dup treat it as a stack value
 * (rc=0 → no-op). This avoids use-after-free when the pool buffer is reused. */
static void pool_ensure(void) {
  if (!pool_base) {
    /* +8 per slot for the RC header; worst case POOL_CAPACITY slots */
    pool_base = (double *)malloc((POOL_CAPACITY + 1) * sizeof(double) * 2);
  }
}

/* Allocate n doubles from the pool (zeroed). Prepends an 8-byte RC header
 * with rc=0 (stack semantics) so perceus won't free or dup the buffer.
 * Returns a _DoubleArray pointing past the header. */
_DoubleArray pool_alloc(int32_t n) {
  pool_ensure();
  /* Reserve 8 bytes (2 doubles worth) for the RC header before the payload */
  int32_t aligned_cursor = (pool_cursor + 1) & ~1;  /* align to 2 doubles */
  if (aligned_cursor + n + 2 > POOL_CAPACITY * 2) {
    aligned_cursor = 0;
  }
  /* RC header: rc=0 (stack, no free), tag=0 */
  double *header = pool_base + aligned_cursor;
  ((int32_t *)header)[0] = 0;  /* rc = 0 (stack) */
  ((int32_t *)header)[1] = 0;  /* tag = 0 */
  double *ptr = pool_base + aligned_cursor + 2;  /* skip 8-byte header */
  pool_cursor = aligned_cursor + 2 + n;
  for (int32_t i = 0; i < n; i++) {
    ptr[i] = 0.0;
  }
  return (_DoubleArray){n, 0, ptr};
}

/* Reset the pool cursor — all previously-allocated arrays are invalidated.
 * Call this at the start of each forward pass. */
void pool_reset(void) {
  pool_cursor = 0;
}
