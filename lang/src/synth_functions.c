#include "synth_functions.h"
#include "value.h"
#include <audio_loop.h>
#include <ctx.h>
#include <node.h>
#include <oscillator.h>

static Value _init_audio(int argc, Value *argv) { return INT(init_audio()); }

static Value _sq(int argc, Value *argv) {
  double freq = argv->value.vnum;
  return OBJ(sq_node(freq));
}

static Value _sin(int argc, Value *argv) {
  double freq = argv->value.vnum;
  return OBJ(sine(freq));
}

static Value _ctx_add(int argc, Value *argv) {
  void *node = argv->value.vobj;
  return OBJ(ctx_add(node));
}

static Value _add_to_dac(int argc, Value *argv) {
  void *node = argv->value.vobj;
  return OBJ(add_to_dac(node));
}

#define SYNTH_FNS 5
static native_symbol_map builtin_native_fns[SYNTH_FNS] = {
    {"Synth.init_audio", NATIVE_FN(_init_audio, 1)},
    {"ctx_add", NATIVE_FN(_ctx_add, 1)},
    {"add_to_dac", NATIVE_FN(_add_to_dac, 1)},
    {"sq", NATIVE_FN(_sq, 1)},
    {"sin", NATIVE_FN(_sin, 1)},
};

void add_synth_functions(ht *stack) {
  for (int i = 0; i < SYNTH_FNS; i++) {
    native_symbol_map t = builtin_native_fns[i];
    ht_set(stack, t.id, t.type);
  }
}
