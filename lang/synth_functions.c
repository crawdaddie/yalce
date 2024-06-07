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

static Value _init_audio(int argc, Value *argv) { return INT(init_audio()); }

#define NODE(i)                                                                \
  (Value) {                                                                    \
    VALUE_SYNTH_NODE, { .vnode = i }                                           \
  }

#define NODE_OF_VALUE(v) v->value.vnode
#define NUM_OF_VALUE(v)                                                        \
  v->type == VALUE_INT ? (double)v->value.vint : v->value.vnum

#define INT_OF_VALUE(v) v->value.vint

static Value _sq(int argc, Value *argv) {
  Value freq = argv[0];
  double *input;
  if (freq.type == VALUE_SYNTH_NODE) {
    Node *freq_input_node = freq.value.vnode;
    input = freq_input_node->output_buf;
  } else if (freq.type == VALUE_INT || freq.type == VALUE_NUMBER) {
    input = malloc(sizeof(double) * BUF_SIZE);
    for (int i = 0; i < BUF_SIZE; i++) {
      input[i] = NUM_OF_VALUE(argv);
    }
  } else {
    fprintf(stderr, "Error: incompatible input to synth function ");
    print_value(argv);
    printf("\n");
    return VOID;
  }
  Node *sq_node = malloc(sizeof(Node));
  sq_node->perform = sq_perform;
  sq_node->state = malloc(sizeof(sq_state));

  ((sq_state *)sq_node->state)->phase = 0.0;

  sq_node->ins = malloc(sizeof(double *) * 1);
  sq_node->ins[0] = input;
  sq_node->num_ins = 1;
  ctx_add(sq_node);

  return (Value){VALUE_SYNTH_NODE, {.vnode = sq_node}};
}

static Value _sin(int argc, Value *argv) {
  Value freq = argv[0];
  double *input;
  if (freq.type == VALUE_SYNTH_NODE) {
    Node *freq_input_node = freq.value.vnode;
    input = freq_input_node->output_buf;
  } else if (freq.type == VALUE_INT || freq.type == VALUE_NUMBER) {
    input = malloc(sizeof(double) * BUF_SIZE);
    for (int i = 0; i < BUF_SIZE; i++) {
      input[i] = NUM_OF_VALUE(argv);
    }
  } else {
    fprintf(stderr, "Error: incompatible input to synth function ");
    print_value(argv);
    printf("\n");
    return VOID;
  }
  Node *sin_node = malloc(sizeof(Node));
  sin_node->perform = sin_perform;
  sin_node->state = malloc(sizeof(sin_state));

  ((sq_state *)sin_node->state)->phase = 0.0;

  sin_node->ins = malloc(sizeof(double *) * 1);
  sin_node->ins[0] = input;
  sin_node->num_ins = 1;
  ctx_add(sin_node);

  return (Value){VALUE_SYNTH_NODE, {.vnode = sin_node}};
}

static Value _play_node(int argc, Value *argv) {
  print_value(argv);
  Node *node = argv->value.vnode;

  node->type = OUTPUT;

  return *argv;
}

static Value _group_add_tail(int argc, Value *argv) {
  group_add_tail(NODE_OF_VALUE(argv), NODE_OF_VALUE((argv + 1)));
  return VOID;
}

Value synth_add(Value l, Value r) {
  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {
    Node *ln = l.value.vnode;
    Node *rn = r.value.vnode;
    Node *sum = malloc(sizeof(Node));
    double **ins = malloc(sizeof(double *) * 2);
    *sum = (Node){
        .perform = sum_perform,
        .ins = ins,
        .num_ins = 2,
    };
    sum->ins[0] = ln->output_buf;
    sum->ins[1] = rn->output_buf;
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

  Node *sum = malloc(sizeof(Node));
  double **ins = malloc(sizeof(double *) * 2);
  *sum = (Node){
      .perform = sum_perform,
      .ins = ins,
      .num_ins = 2,
  };

  sum->ins[0] = malloc(sizeof(double) * BUF_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    sum->ins[0][i] = scalar;
  }
  sum->ins[1] = n->output_buf;
  ctx_add(sum);
  return NODE(sum);
}

Value synth_mul(Value l, Value r) {
  if (l.type == VALUE_SYNTH_NODE && r.type == VALUE_SYNTH_NODE) {
    Node *ln = l.value.vnode;
    ctx_add(ln);
    Node *rn = r.value.vnode;
    ctx_add(rn);
    Node *mul = malloc(sizeof(Node));
    double **ins = malloc(sizeof(double *) * 2);
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
  Node *mul = malloc(sizeof(Node));
  double **ins = malloc(sizeof(double *) * 2);
  *mul = (Node){
      .perform = mul_perform,
      .ins = ins,
      .num_ins = 2,
  };

  mul->ins[0] = malloc(sizeof(double) * BUF_SIZE);
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
static SynthContext _synth_context = {.head = NULL, .tail = NULL};

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
