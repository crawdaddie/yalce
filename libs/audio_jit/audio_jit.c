#include "./audio_jit.h"

#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/common.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./compile_synth.h"
#include "./dsp_fn_application.h"
#include "pattern_coroutine.h"

#include <llvm-c/Core.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
int STYPE_AUDIO_JIT_INLINE_LAMBDA;
int STYPE_AUDIO_JIT_SYNTH_INLET;
int STYPE_AUDIO_JIT_LOCAL_ARRAY;

// let array_of_buf = extern fn Ptr -> Array of Double;
// let bufsize = extern fn Ptr -> Int;
_DoubleArray array_of_buf(NodeRef buf) {
  return (_DoubleArray){.data = buf->output.buf, .size = buf->output.size};
}
int bufsize(NodeRef buf) { return buf->output.size; }
void *ylc_get_output_buf(void *node_raw) {
  return ((Node *)node_raw)->output.buf;
}
int64_t ylc_bufsize(void *node_raw) {
  return (int64_t)((Node *)node_raw)->output.size;
}

Node *ylc_create_audio_node(perform_func_t perform, int num_inputs,
                            int state_bytes, const char *meta_name) {
  size_t total =
      sizeof(Node) + (size_t)state_bytes + ((size_t)BUF_SIZE * sizeof(double));
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return NULL;
  }

  node->perform = perform;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
  node->meta = (char *)meta_name;
  node->output = (Signal){
      .layout = 1,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->next = NULL;

  return node;
}

void ylc_register_synth_ctor(int synth_id, void *ctor) {
  synth_registry_set_ctor_ptr(synth_id, ctor);
}

void *ylc_get_synth_ctor(int synth_id) {
  return synth_registry_get_ctor_ptr(synth_id);
}

int ylc_rand_int(int n) {
  if (n <= 1)
    return 0;
  return rand() % n;
}

Node *ylc_const_inlet(double val) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);
  int saved_idx = node->node_index;

  *node = (Node){
      .perform = NULL,
      .node_index = saved_idx,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = (char *)"jit_const_inlet",
  };

  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.buf[i] = val;
  }

  return node;
}

static void register_builtin(ht *stack, const char *name,
                             BuiltinHandler handler) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = handler;
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
}

__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  init_synth_registry();

  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no JIT context at load time\n");
    return;
  }

  STYPE_AUDIO_JIT_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_BUILTIN_HANDLER = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_INLINE_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_INLINE_LAMBDA = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_SYNTH_INLET = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_LOCAL_ARRAY = REGISTERED_JIT_SYMBOL_TYPE++;

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "compile_audio_fn", CompileAudioFnHandler);
  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");
  printf("sample rates: %d %f\n", ctx_sample_rate(), ctx_spf());

  register_builtin(stack, "pat", pattern_handler);
  register_builtin(stack, "pat_key", pattern_key_handler);
}
