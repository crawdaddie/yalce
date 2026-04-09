
#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/module.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/common.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/types/type.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include "./compile_synth_mlir.hpp"
#include "./synth_record.h"

#include <llvm-c/Core.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./lib.h"

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;
int STYPE_AUDIO_JIT_INLINE_LAMBDA;
int STYPE_AUDIO_JIT_SYNTH_INLET;
int STYPE_AUDIO_JIT_LOCAL_ARRAY;

static void **g_mlir_global_storage_array = NULL;
static int *g_mlir_global_storage_capacity = NULL;

void ylc_mlir_set_global_storage(void **storage_array, int *storage_capacity) {
  g_mlir_global_storage_array = storage_array;
  g_mlir_global_storage_capacity = storage_capacity;
}

double ylc_mlir_get_global_f64(int slot) {
  if (!g_mlir_global_storage_array || slot < 0 ||
      (g_mlir_global_storage_capacity &&
       slot >= *g_mlir_global_storage_capacity) ||
      !g_mlir_global_storage_array[slot]) {
    return 0.0;
  }
  return *(double *)g_mlir_global_storage_array[slot];
}

int ylc_mlir_get_global_i32(int slot) {
  if (!g_mlir_global_storage_array || slot < 0 ||
      (g_mlir_global_storage_capacity &&
       slot >= *g_mlir_global_storage_capacity) ||
      !g_mlir_global_storage_array[slot]) {
    return 0;
  }
  return *(int *)g_mlir_global_storage_array[slot];
}

int ylc_mlir_get_global_i1(int slot) {
  if (!g_mlir_global_storage_array || slot < 0 ||
      (g_mlir_global_storage_capacity &&
       slot >= *g_mlir_global_storage_capacity) ||
      !g_mlir_global_storage_array[slot]) {
    return 0;
  }
  return *(int *)g_mlir_global_storage_array[slot] ? 1 : 0;
}

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
  mlir_registry_set_ctor_ptr(synth_id, ctor);
}

void *ylc_get_synth_ctor(int synth_id) {
  return mlir_registry_get_ctor_ptr(synth_id);
}

void ylc_register_synth_frame(int synth_id, void *frame) {
  mlir_registry_set_frame_ptr(synth_id, frame);
}

void *ylc_get_synth_frame(int synth_id) {
  return mlir_registry_get_frame_ptr(synth_id);
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

static LLVMValueRef call_mlir_dsp_symbol(Ast *ast, JITLangCtx *ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  Ast *sym_id = ast->data.AST_APPLICATION.function;
  JITSymbol *sym = lookup_id_ast(sym_id, ctx);
  if (!sym) {
    fprintf(stderr, "audio_jit_mlir: unresolved symbol\n");
    return LLVMConstNull(GENERIC_PTR);
  }

  int synth_id = sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
  MlirSynthRecord rec = mlir_registry_get(synth_id);
  int num_inputs = rec.num_inputs;

  // ctor signature: (f64 x num_inputs) -> ptr
  LLVMTypeRef *param_tys =
      num_inputs ? alloca(sizeof(LLVMTypeRef) * (size_t)num_inputs) : NULL;
  for (int i = 0; i < num_inputs; i++)
    param_tys[i] = LLVMDoubleType();
  LLVMTypeRef ctor_fn_ty =
      LLVMFunctionType(GENERIC_PTR, param_tys, (unsigned)num_inputs, 0);

  // Emit a runtime lookup so hot-reload picks up new ctor_ptr automatically.
  LLVMTypeRef get_ctor_fn_ty =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
  LLVMValueRef get_ctor_fn =
      LLVMGetNamedFunction(module, "mlir_registry_get_ctor_ptr");
  if (!get_ctor_fn) {
    get_ctor_fn =
        LLVMAddFunction(module, "mlir_registry_get_ctor_ptr", get_ctor_fn_ty);
    LLVMSetLinkage(get_ctor_fn, LLVMExternalLinkage);
  }
  LLVMValueRef synth_id_val =
      LLVMConstInt(LLVMInt32Type(), (unsigned long long)synth_id, 0);
  LLVMValueRef ctor_ptr = LLVMBuildCall2(builder, get_ctor_fn_ty, get_ctor_fn,
                                         &synth_id_val, 1, "mlir.ctor.ptr");

  // Collect and coerce arguments.
  int arg_count = ast->data.AST_APPLICATION.len;
  LLVMValueRef *ctor_args =
      num_inputs ? alloca(sizeof(LLVMValueRef) * (size_t)num_inputs) : NULL;
  for (int i = 0; i < num_inputs; i++) {
    if (i < arg_count) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + i;
      LLVMValueRef v = codegen(arg_ast, ctx, module, builder);
      v = handle_type_conversions(v, arg_ast->type, &t_num, ctx, module,
                                  builder);
      ctor_args[i] = v;
    } else {
      ctor_args[i] = LLVMConstReal(LLVMDoubleType(), 0.0);
    }
  }

  // Prevent the caller from converting the Node* return to a const signal.
  ast->type = &t_ptr;

  return LLVMBuildCall2(builder, ctor_fn_ty, ctor_ptr, ctor_args,
                        (unsigned)num_inputs, "mlir.node");
}

static void bind_compiled_mlir_synth(Ast *binding, JITLangCtx *ctx, int new_id) {
  const char *name = binding->data.AST_IDENTIFIER.value;
  JITSymbol *existing = lookup_id_ast(binding, ctx);
  int synth_id;
  if (existing && existing->type == (symbol_type)STYPE_AUDIO_JIT_SYM) {
    synth_id = existing->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
    MlirSynthRecord rec = mlir_registry_get(new_id);
    ylc_register_synth_ctor(synth_id, rec.ctor_ptr);
    ylc_register_synth_frame(synth_id, rec.frame_ptr);
  } else {
    synth_id = new_id;
  }

  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
      call_mlir_dsp_symbol;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = synth_id;
  ht_set_hash(ctx->frame->table, name, hash_string(name, strlen(name)), sym);
}

static int synth_num_inputs(Ast *lambda) {
  int num_inputs = 0;
  if (!lambda || is_void_func(lambda->type))
    return 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast->tag == AST_IDENTIFIER)
      num_inputs++;
    else if (p->ast->tag == AST_TUPLE)
      num_inputs += p->ast->data.AST_LIST.len;
  }
  return num_inputs;
}

static Ast *group_compile_source_body(Ast *source) {
  if (!source)
    return NULL;
  if (source->tag == AST_BODY)
    return source;
  if (source->tag == AST_MODULE)
    return source->data.AST_LAMBDA.body;
  if (source->tag == AST_LET && source->data.AST_LET.expr &&
      source->data.AST_LET.expr->tag == AST_MODULE)
    return source->data.AST_LET.expr->data.AST_LAMBDA.body;
  return NULL;
}

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  ylc_mlir_set_global_storage(ctx->global_storage_array,
                              ctx->global_storage_capacity);

  Ast *source = ast->data.AST_APPLICATION.args;
  Ast *group_body = group_compile_source_body(source);
  if (group_body) {
    JITLangCtx *compile_ctx = ctx;
    JITSymbol *module_symbol = NULL;
    Ast *module_binding = NULL;
    Ast *module_ast = NULL;

    if (source->tag == AST_LET && source->data.AST_LET.expr &&
        source->data.AST_LET.expr->tag == AST_MODULE) {
      module_binding = source->data.AST_LET.binding;
      module_ast = source->data.AST_LET.expr;
      module_symbol =
          create_module_symbol(module_ast->type, NULL, module_ast, ctx, module);
      compile_ctx = module_symbol->symbol_data.STYPE_MODULE.ctx;

      const char *mod_binding = module_binding->data.AST_IDENTIFIER.value;
      ht_set_hash(ctx->frame->table, mod_binding,
                  hash_string(mod_binding, strlen(mod_binding)), module_symbol);
    }

    int first_id = mlir_registry_len();
    AST_LIST_ITER(group_body->data.AST_BODY.stmts, ({
      Ast *stmt = l->ast;
      if (!stmt || stmt->tag != AST_LET)
        continue;
      Ast *binding = stmt->data.AST_LET.binding;
      Ast *lambda = stmt->data.AST_LET.expr;
      if (!binding || binding->tag != AST_IDENTIFIER || !lambda ||
          lambda->tag != AST_LAMBDA)
        continue;

      MlirSynthRecord rec = {0};
      rec.name = strdup(binding->data.AST_IDENTIFIER.value);
      rec.num_inputs = synth_num_inputs(lambda);
      int id = mlir_registry_extend(rec);
      bind_compiled_mlir_synth(binding, compile_ctx, id);
    }));

    int count = 0;
    int compiled_first_id =
        mlir_compile_synth_group_into(group_body, compile_ctx, &count, first_id);
    if (compiled_first_id < 0) {
      fprintf(stderr, "audio_jit_mlir: grouped compile failed\n");
      return NULL;
    }

    int idx = 0;
    AST_LIST_ITER(group_body->data.AST_BODY.stmts, ({
      Ast *stmt = l->ast;
      if (!stmt || stmt->tag != AST_LET)
        continue;
      Ast *binding = stmt->data.AST_LET.binding;
      Ast *lambda = stmt->data.AST_LET.expr;
      if (!binding || binding->tag != AST_IDENTIFIER || !lambda ||
          lambda->tag != AST_LAMBDA)
        continue;
      bind_compiled_mlir_synth(binding, compile_ctx, compiled_first_id + idx);
      idx++;
    }));
    return NULL;
  }

  Ast *lambda = source->data.AST_LET.expr;
  Ast *binding = source->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  int num_inputs = synth_num_inputs(lambda);

  int new_id = mlir_compile_synth(lambda, name, ctx, num_inputs);
  if (new_id < 0) {
    fprintf(stderr, "audio_jit_mlir: compile failed for '%s'\n", name);
    return NULL;
  }
  bind_compiled_mlir_synth(binding, ctx, new_id);
  return NULL;
}
__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  mlir_registry_init();

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
}
