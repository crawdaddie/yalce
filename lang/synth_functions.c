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
  current_synth_ctx.tail->next = node;
  current_synth_ctx.tail = node;
  return node;
}

void *alloc(AllocArena *arena, unsigned long size) {
  void *ptr = arena->blob_ptr;

  unsigned long offset = ptr - arena->blob;

  // if (offset + size > arena->capacity) {
  //   printf("realloc to accommodate %d from %d\n", offset + size,
  //          arena->capacity);
  //   arena->capacity *= 2;
  //   arena->blob = realloc(arena->blob, arena->capacity);
  //   arena->blob_ptr = arena->blob + offset;
  //   ptr = arena->blob_ptr;
  // }

  arena->blob_ptr += size;
  return ptr;
}

AllocArena get_arena(int size) {
  void *mem = malloc(size + 4);
  // printf("size %lu\n", size * sizeof(void *));
  return (AllocArena){.blob = mem, .blob_ptr = mem, .capacity = size};
}

double *const_signal(AllocArena *arena, double v) {
  double *input = alloc(arena, SIGNAL_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    input[i] = v;
  }
  return input;
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

  int size = sizeof(Node) + sizeof(sq_state) + SIGNAL_SIZE;
  if (input_is_sig) {
    size += SIGNAL_SIZE;
  }
  AllocArena arena = get_arena(size);

  double *input;
  Node *sq_node = alloc(&arena, sizeof(Node));
  sq_node->perform = sq_perform;
  sq_node->state = alloc(&arena, sizeof(sq_state));

  if (input_is_sig) {
    Node *freq_input_node = freq.value.vnode;
    input = freq_input_node->output_buf;
  } else if (freq.type == VALUE_INT || freq.type == VALUE_NUMBER) {
    input = const_signal(&arena, NUM_OF_VALUE(argv));
  }

  ((sq_state *)sq_node->state)->phase = 0.0;

  sq_node->ins = alloc(&arena, sizeof(double *) * 1);
  sq_node->ins[0] = input;
  sq_node->num_ins = 1;
  // printf("sq %p\n", sq_node);
  ctx_add(sq_node);

  return (Value){VALUE_SYNTH_NODE, {.vnode = sq_node}};
}

static Value _sin(Value *argv) {
  Value freq = argv[0];
  if (!is_synth_input(freq)) {
    fprintf(stderr, "Error: incompatible input to synth function ");
    fprint_value(stderr, argv);
    return VOID;
  }

  int input_is_sig = freq.type == VALUE_SYNTH_NODE;

  int size = sizeof(Node) + sizeof(sq_state) + SIGNAL_SIZE;
  if (input_is_sig) {
    size += SIGNAL_SIZE;
  }
  AllocArena arena = get_arena(size);
  double *input;

  Node *sin_node = alloc(&arena, sizeof(Node));
  sin_node->perform = sin_perform;
  sin_node->state = alloc(&arena, sizeof(sin_state));

  if (input_is_sig) {
    Node *freq_input_node = freq.value.vnode;
    input = freq_input_node->output_buf;
  } else if (freq.type == VALUE_INT || freq.type == VALUE_NUMBER) {
    input = const_signal(&arena, NUM_OF_VALUE(argv));
  }

  ((sq_state *)sin_node->state)->phase = 0.0;

  sin_node->ins = alloc(&arena, sizeof(double *) * 1);
  sin_node->ins[0] = input;
  sin_node->num_ins = 1;
  ctx_add(sin_node);

  return (Value){VALUE_SYNTH_NODE, {.vnode = sin_node}};
}

static Node *make_group(SynthCtx sctx) {
  Node *group = malloc(sizeof(Node));
  group_state *gr = (group_state *)malloc(sizeof(group_state));
  group->state = gr;
  ((group_state *)group->state)->head = sctx.head;
  ((group_state *)group->state)->tail = sctx.tail;
  group->perform = group_perform;
  group->num_ins = 0;
  group->ins = NULL;
  return group;
}

static Value __play_node(Value *argv) {
  // printf("head %p tail %p num %d\n", current_synth_ctx.head,
  //        current_synth_ctx.tail, current_synth_ctx.num);

  Node *node = argv->value.vnode;

  struct timespec bt = get_block_time();
  struct timespec now;
  set_block_time(&now);

  Ctx *ctx = get_audio_ctx();
  int frame_offset = get_block_frame_offset(bt, now, ctx->sample_rate);
  if (current_synth_ctx.num == 1) {
    push_msg(&ctx->msg_queue, (scheduler_msg){
                                  .type = NODE_ADD,
                                  .frame_offset = frame_offset,
                                  .body = {.NODE_ADD = node},
                              });
  } else if (current_synth_ctx.num > 1) {
    Node *group = make_group(current_synth_ctx);

    push_msg(&ctx->msg_queue, (scheduler_msg){
                                  NODE_ADD,
                                  frame_offset,
                                  .body = {.NODE_ADD = current_synth_ctx.head},
                              });

    current_synth_ctx = (SynthCtx){};
    return NODE(group);
  }

  current_synth_ctx = (SynthCtx){};
  return *argv;
}

void add_node_msg(Node *node, int frame_offset) {
  Ctx *ctx = get_audio_ctx();
  push_msg(&ctx->msg_queue,
           (scheduler_msg){NODE_ADD, frame_offset,
                           .body = {.NODE_ADD = {.target = node}}});
}
void add_group_msg(Node *head, Node *tail, int frame_offset) {
  Ctx *ctx = get_audio_ctx();
  push_msg(
      &ctx->msg_queue,
      (scheduler_msg){GROUP_ADD, frame_offset,
                      .body = {.GROUP_ADD = {.head = head, .tail = tail}}});
}

static Value _play_node(Value *argv) {
  // printf("head %p tail %p num %d\n", current_synth_ctx.head,
  //        current_synth_ctx.tail, current_synth_ctx.num);

  Node *node = argv->value.vnode;

  struct timespec bt = get_block_time();
  struct timespec now;
  set_block_time(&now);

  Ctx *ctx = get_audio_ctx();
  int frame_offset = get_block_frame_offset(bt, now, ctx->sample_rate);
  // current_synth_ctx.tail->type = OUTPUT;
  // audio_ctx_add(current_synth_ctx.head);
  //
  //
  if (current_synth_ctx.num > 1) {

    // node->type = OUTPUT;

    // Node *group = make_group(current_synth_ctx);
    // printf("group %p\n", group);
    // group->frame_offset = frame_offset;

    // audio_ctx_add(current_synth_ctx.head);
    add_group_msg(current_synth_ctx.head, node, frame_offset);

    current_synth_ctx = (SynthCtx){};
    return *argv;
  }

  // node->type = OUTPUT;
  add_node_msg(node, frame_offset);
  current_synth_ctx = (SynthCtx){};
  return *argv;
}

static Value _group_add_tail(Value *argv) {
  group_add_tail(NODE_OF_VALUE(argv), NODE_OF_VALUE((argv + 1)));
  return VOID;
}

Value synth_add(Value l, Value r) {

  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {
    AllocArena arena = get_arena(sizeof(Node) + sizeof(double *) * 2);
    Node *ln = l.value.vnode;
    Node *rn = r.value.vnode;
    Node *sum = alloc(&arena, sizeof(Node));
    double **ins = alloc(&arena, sizeof(double *) * 2);
    sum->perform = sum_perform;
    sum->ins = ins;
    sum->num_ins = 2;
    sum->ins[0] = ln->output_buf;
    sum->ins[1] = rn->output_buf;

    // printf("sum %p\n", sum);
    ctx_add(sum);
    return NODE(sum);
  }

  AllocArena arena =
      get_arena(sizeof(Node) + (sizeof(double *) * 2) + SIGNAL_SIZE);

  Node *n;
  double scalar;
  if (l.type != VALUE_SYNTH_NODE) {
    scalar = l.type == VALUE_INT ? l.value.vint : l.value.vnum;
    n = r.value.vnode;
  } else if (r.type != VALUE_SYNTH_NODE) {
    scalar = r.type == VALUE_INT ? r.value.vint : r.value.vnum;
    n = l.value.vnode;
  }

  Node *sum = alloc(&arena, sizeof(Node));
  double **ins = alloc(&arena, sizeof(double *) * 2);

  sum->perform = sum_perform;
  sum->ins = ins;
  sum->num_ins = 2;

  sum->ins[0] = alloc(&arena, SIGNAL_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    sum->ins[0][i] = scalar;
  }
  sum->ins[1] = n->output_buf;

  // printf("sum %p\n", sum);
  ctx_add(sum);
  return NODE(sum);
}

Value synth_mul(Value l, Value r) {
  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {

    AllocArena arena = get_arena(sizeof(Node) + sizeof(double *) * 2);
    Node *ln = l.value.vnode;
    ctx_add(ln);
    Node *rn = r.value.vnode;
    ctx_add(rn);
    Node *mul = alloc(&arena, sizeof(Node));
    double **ins = alloc(&arena, sizeof(double *) * 2);
    *mul = (Node){
        .perform = mul_perform,
        .ins = ins,
        .num_ins = 2,
    };
    mul->ins[0] = ln->output_buf;
    mul->ins[1] = rn->output_buf;
    ctx_add(mul);
    return NODE(mul);
  }

  AllocArena arena =
      get_arena(sizeof(Node) + sizeof(double *) * 2 + SIGNAL_SIZE);

  Node *n;
  double scalar;
  if (l.type != VALUE_SYNTH_NODE) {
    // l is a scalar

    scalar = l.type == VALUE_INT ? l.value.vint : l.value.vnum;
    n = r.value.vnode;
    // ctx_add(n);
  } else if (r.type != VALUE_SYNTH_NODE) {
    // r is a scalar
    scalar = r.type == VALUE_INT ? r.value.vint : r.value.vnum;
    n = l.value.vnode;
  }
  Node *mul = alloc(&arena, sizeof(Node));
  double **ins = alloc(&arena, sizeof(double *) * 2);
  *mul = (Node){
      .perform = mul_perform,
      .ins = ins,
      .num_ins = 2,
  };

  mul->ins[0] = alloc(&arena, SIGNAL_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    mul->ins[0][i] = scalar;
  }
  mul->ins[1] = n->output_buf;
  ctx_add(mul);
  return NODE(mul);
  // TODO: handle scalar * node
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
// static SynthContext _synth_context = {.head = NULL, .tail = NULL};

static Value synth_val_bind(Value val) {
  // if (val.type == VALUE_SYNTH_NODE) {
  //   if (((Graph *)_graph_context.current_graph->state)->tail !=
  //       val.value.vnode) {
  //     group_add_tail(_graph_context.current_graph, val.value.vnode);
  //     _graph_context.num_nodes++;
  //   }
  // }
  // return val;
  return VOID;
}

static Value _synth_wrapper_meta(Ast *ast, LangCtx *ctx) {
  if (ast->tag == AST_LET && ast->data.AST_LET.expr->tag == AST_LAMBDA) {
    Ast *lambda = ast->data.AST_LET.expr;

    Value synth_func_ = eval_lambda_declaration(ast->data.AST_LET.expr, ctx);

    Function synth_def = synth_func_.value.function;

    Value *args = malloc(sizeof(Value) * synth_def.len);

    for (int i = 0; i < synth_def.len; i++) {
      Ast *default_arg = lambda->data.AST_LAMBDA.defaults[i];
      if (default_arg != NULL) {
        args[i] = eval(default_arg, ctx);
      } else {
        args[i] = NUM(0);
      }
    }

    // _graph_context.current_graph = group_new(1);
    // _graph_context.num_nodes = 0;
    LangCtx new_ctx = {
        .stack = ctx->stack,
        .stack_ptr = ctx->stack_ptr,
        .val_bind = synth_val_bind,
    };

    Value result_node = fn_call(synth_def, args, &new_ctx);

    // add_to_dac(result_node.value.vnode);

    // flatten_synth(_graph_context.num_nodes, _graph_context.current_graph);

    // Return new function that returns a copy of current_graph
    // return NODE(current_graph);
    Value *synth_func = malloc(sizeof(Value));
    *synth_func = (Value){.type = VALUE_NATIVE_FN, .value = {.native_fn = {}}};
    ht_set_hash(ctx->stack + ctx->stack_ptr, ast->data.AST_LET.name.chars,
                ast->data.AST_LET.name.hash, synth_func);
    return *synth_func;
  }
  return eval(ast, ctx);
}

static Value synth_wrapper_meta(Ast *ast, LangCtx *ctx) {
  if (!(ast->tag == AST_LET && ast->data.AST_LET.expr->tag == AST_LAMBDA)) {
    return eval(ast, ctx);
  }

  Ast *lambda = ast->data.AST_LET.expr;
  Value synth_def_func = eval_lambda_declaration(ast->data.AST_LET.expr, ctx);

  int synth_def_len = lambda->data.AST_LAMBDA.len;
  Value *args = malloc(sizeof(Value) * (synth_def_len + 1));
  args[0] = synth_def_func;

  Value mk_synth = (Value){VALUE_NATIVE_FN,
                           {.native_fn = {.len = synth_def_len + 1,
                                          .num_partial_args = 1,
                                          .partial_args = args}}};
  return mk_synth;
}

static ht synth_blobs;
void add_synth_functions(ht *stack) {
  for (int i = 0; i < SYNTH_FNS; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }

  ht_set(stack, "@synth",
         &(Value){VALUE_META_FN, {.vmeta_fn = &synth_wrapper_meta}});
  ht_init(&synth_blobs);
}
#undef NODE
#undef NODE_OF_VALUE
#undef INT_OF_VALUE
