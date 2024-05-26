#include "synth_functions.h"
#include "value.h"
#include <audio_loop.h>
#include <ctx.h>
#include <node.h>
#include <oscillator.h>

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
  Value sq = NODE(sq_node(NUM_OF_VALUE(argv)));
  return sq;
}

static Value _sin(int argc, Value *argv) {
  return NODE(sine(NUM_OF_VALUE(argv)));
}

static Value _ctx_add(int argc, Value *argv) {
  return NODE(ctx_add(NODE_OF_VALUE(argv)));
}

static Value _add_to_dac(int argc, Value *argv) {
  return NODE(add_to_dac(NODE_OF_VALUE(argv)));
}
static Value _group_new(int argc, Value *argv) {
  return NODE(group_new(INT_OF_VALUE(argv)));
}

static Value _group_add_tail(int argc, Value *argv) {
  group_add_tail(NODE_OF_VALUE(argv), NODE_OF_VALUE((argv + 1)));
  return VOID;
}

// static Value _sum_nodes_arr(int argc, Value *nodes) {
//   group_add_tail(NODE_OF_VALUE(argv), NODE_OF_VALUE((argv + 1)));
//   return VOID;
// }

Value synth_add(Value l, Value r) { return VOID; }

#define SYNTH_FNS 7
static native_symbol_map builtin_native_fns[SYNTH_FNS] = {
    {"init_audio", NATIVE_FN(_init_audio, 1)},
    {"ctx_add", NATIVE_FN(_ctx_add, 1)},
    {"add_to_dac", NATIVE_FN(_add_to_dac, 1)},
    {"sq", NATIVE_FN(_sq, 1)},
    {"sin", NATIVE_FN(_sin, 1)},
    {"group_new", NATIVE_FN(_group_new, 1)},
    {"group_add_tail", NATIVE_FN(_group_add_tail, 2)},
};

void add_synth_functions(ht *stack) {
  for (int i = 0; i < SYNTH_FNS; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }
}
#undef NODE
#undef NODE_OF_VALUE
