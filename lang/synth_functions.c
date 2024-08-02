#include "synth_functions.h"
#include "ctx.h"
#include "eval.h"
#include "eval_function.h"
#include "serde.h"
#include "value.h"
#include <audio_loop.h>
#include <lib.h>
#include <oscillators.h>
#include <stdlib.h>

static Value _init_audio(Value *argv) { return INT(init_audio()); }

#define NODE(i)                                                                \
  (Value) {                                                                    \
    VALUE_SYNTH_NODE, { .vnode = i }                                           \
  }

#define NODE_OF_VALUE(v) v->value.vnode
#define NUM_OF_VALUE(v)                                                        \
  v->type == VALUE_INT ? (double)v->value.vint : v->value.vnum

#define INT_OF_VALUE(v) v->value.vint
typedef struct {
  void *blob;
  void *blob_ptr;
  int capacity;
} AllocArena;

#define SIGNAL_SIZE sizeof(double) * BUF_SIZE

typedef struct {
  Node *head;
  Node *tail;
  int num;
} SynthCtx;

static SynthCtx current_synth_ctx = {.num = 0};

static Node *ctx_add(Node *node) {
  // printf("adding %p\n", node);
  current_synth_ctx.num++;
  if (current_synth_ctx.head == NULL) {
    current_synth_ctx.head = node;
    current_synth_ctx.tail = node;
    return node;
  }
  Node *tail = current_synth_ctx.tail;
  tail->next = node;
  current_synth_ctx.tail = node;
  return node;
}
static Value node_val(Node *node) {
  ctx_add(node);
  return NODE(node);
}

void *alloc(AllocArena *arena, unsigned long size) {
  void *ptr = arena->blob_ptr;
  unsigned long offset = ptr - arena->blob;
  arena->blob_ptr += size;
  return ptr;
}

AllocArena get_arena(int size) {
  void *mem = malloc(size + 4);
  return (AllocArena){.blob = mem, .blob_ptr = mem, .capacity = size};
}

Signal *const_signal(int layout, double v) {
  return get_sig_default(layout, v);
}

static inline bool is_synth_input(Value i) {
  return (i.type == VALUE_SYNTH_NODE) || (i.type == VALUE_INT) ||
         (i.type == VALUE_NUMBER);
}

static Value _sq(Value *argv) {
  Value freq = argv[0];
  if (!is_synth_input(freq)) {
    fprintf(stderr, "Error: incompatible input to synth function ");
    fprint_value(stderr, argv);
    return VOID;
  }
  int input_is_sig = freq.type == VALUE_SYNTH_NODE;
  Node *sq;
  if (input_is_sig) {
    sq = sq_node_of_scalar(100.);
    sq->ins[0].buf = freq.value.vnode->out.buf;
  } else {
    sq = sq_node_of_scalar(NUM_OF_VALUE(argv));
  }

  return node_val(sq);
}

static Value _sin(Value *argv) {

  Value freq = argv[0];
  if (!is_synth_input(freq)) {
    fprintf(stderr, "Error: incompatible input to synth function ");
    fprint_value(stderr, argv);
    return VOID;
  }
  int input_is_sig = freq.type == VALUE_SYNTH_NODE;
  Node *s;
  if (input_is_sig) {
    s = sin_node_of_scalar(100.);
    s->ins[0].buf = freq.value.vnode->out.buf;
  } else {
    s = sin_node_of_scalar(NUM_OF_VALUE(argv));
  }

  return node_val(s);
}

void add_node_msg(Node *node, int frame_offset) {
  Ctx *ctx = get_audio_ctx();
  push_msg(&ctx->msg_queue,
           (scheduler_msg){NODE_ADD, frame_offset,
                           .body = {.NODE_ADD = {.target = node}}});
}

static Node *make_group(Node *head, Node *tail) {
  Node *group = group_new(0);
  ((group_state *)group->state)->head = head;
  ((group_state *)group->state)->tail = tail;
  return group;
}

Node *add_group_msg(Node *head, Node *tail, int frame_offset) {
  Node *group = make_group(head, tail);
  Ctx *ctx = get_audio_ctx();
  push_msg(&ctx->msg_queue,
           (scheduler_msg){GROUP_ADD, frame_offset,
                           .body = {.GROUP_ADD = {.group = group}}});
  return group;
}

static Value _play_node(Value *argv) {
  Node *node = argv->value.vnode;

  struct timespec bt = get_block_time();
  struct timespec now;
  set_block_time(&now);

  Ctx *ctx = get_audio_ctx();
  int frame_offset = get_block_frame_offset(bt, now, ctx->sample_rate);
  if (current_synth_ctx.num > 1) {
    node->type = OUTPUT;
    Node *g = make_group(current_synth_ctx.head, node);
    g->type = OUTPUT;
    add_node_msg(g, frame_offset);
    current_synth_ctx = (SynthCtx){};
    return node_val(g);
  }

  add_node_msg(node, frame_offset);
  current_synth_ctx = (SynthCtx){};
  return *argv;
}

static Value _group_add_tail(Value *argv) {
  group_add_tail(NODE_OF_VALUE(argv), NODE_OF_VALUE((argv + 1)));
  return VOID;
}

Value add_nodes(Value l, Value r) {

  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {
    Node *ln = l.value.vnode;
    Node *rn = r.value.vnode;
    Node *sum = sum2_node(ln, rn);

    ctx_add(sum);
    return NODE(sum);
  }

  Node *n;
  double scalar;
  if (l.type != VALUE_SYNTH_NODE) {
    scalar = l.type == VALUE_INT ? l.value.vint : l.value.vnum;
    n = r.value.vnode;
  } else if (r.type != VALUE_SYNTH_NODE) {
    scalar = r.type == VALUE_INT ? r.value.vint : r.value.vnum;
    n = l.value.vnode;
  }

  Node *sum = node_new(NULL, (node_perform *)sum_perform, 2, NULL);
  sum->ins = malloc(sizeof(Signal) * 2);
  sum->ins[0].buf = get_sig_default(1, scalar)->buf;
  sum->ins[1].buf = (n->out).buf;

  ctx_add(sum);
  return NODE(sum);
}

Value mul_nodes(Value l, Value r) {

  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {
    Node *ln = l.value.vnode;
    Node *rn = r.value.vnode;
    Node *sum = mul2_node(ln, rn);

    ctx_add(sum);
    return NODE(sum);
  }

  Node *n;
  double scalar;
  if (l.type != VALUE_SYNTH_NODE) {
    scalar = l.type == VALUE_INT ? l.value.vint : l.value.vnum;
    n = r.value.vnode;
  } else if (r.type != VALUE_SYNTH_NODE) {
    scalar = r.type == VALUE_INT ? r.value.vint : r.value.vnum;
    n = l.value.vnode;
  }

  Node *mul = node_new(NULL, (node_perform *)mul_perform, 2, NULL);
  mul->ins = malloc(sizeof(Signal) * 2);
  mul->ins[0].buf = get_sig_default(1, scalar)->buf;
  mul->ins[1].buf = (n->out).buf;

  ctx_add(mul);
  return NODE(mul);
}

#define SYNTH_FNS 5
static native_symbol_map builtin_native_fns[SYNTH_FNS] = {
    {"init_audio", NATIVE_FN(_init_audio, 1)},
    // {"ctx_add", NATIVE_FN(_ctx_add, 1)},
    // {"add_to_dac", NATIVE_FN(_add_to_dac, 1)},
    // {"group_new", NATIVE_FN(_group_new, 1)},
    {"group_add_tail", NATIVE_FN(_group_add_tail, 2)},
    {"sq", NATIVE_FN(_sq, 1)},
    {"sin", NATIVE_FN(_sin, 1)},
    {"play_node", NATIVE_FN(_play_node, 1)},
};

typedef struct {
  Node *current_graph;
  int num_nodes;
} GraphContext;

typedef struct {
  Node *head;
  Node *tail;
} SynthContext;

static GraphContext _graph_context = {.current_graph = NULL, .num_nodes = 0};

static ht synth_blobs;
void add_synth_functions(ht *stack) {
  for (int i = 0; i < SYNTH_FNS; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }

  // ht_set(stack, "@synth",
  //        &(Value){VALUE_META_FN, {.vmeta_fn = &synth_wrapper_meta}});
  ht_init(&synth_blobs);
}
#undef NODE
#undef NODE_OF_VALUE
#undef INT_OF_VALUE
