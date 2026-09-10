#include "audio_jit_mlir.h"
#include "mlir_synth_compiler.h"

extern "C" {
#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/ht.h"
#include "../../lang/types/builtins.h"
}

#include "llvm-c/Core.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

int STYPE_AUDIO_JIT_MLIR_SYM;

namespace {

struct MlirSynthRecord {
  std::string public_name;
  ylc::audio_mlir::MlirSynthNames names;
  LLVMValueRef cons_fn = nullptr;
  LLVMValueRef init_fn = nullptr;
  LLVMValueRef perform_fn = nullptr;
  LLVMValueRef frame_fn = nullptr;
  std::atomic<void *> ctor_ptr{nullptr};
  unsigned arg_count = 0;
  int output_lanes = 1;
  int state_bytes = 0;
};

std::vector<std::unique_ptr<MlirSynthRecord>> g_synths;
static int g_symbol_counter = 0;

static LLVMTypeRef i8_type(LLVMModuleRef module) {
  return LLVMInt8TypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef i32_type(LLVMModuleRef module) {
  return LLVMInt32TypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef f64_type(LLVMModuleRef module) {
  return LLVMDoubleTypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef void_type(LLVMModuleRef module) {
  return LLVMVoidTypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef ptr_type(LLVMModuleRef module) {
  return LLVMPointerType(i8_type(module), 0);
}

static std::string ast_identifier_string(const Ast *id) {
  if (!id || id->tag != AST_IDENTIFIER || !id->data.AST_IDENTIFIER.value) {
    return "<anonymous>";
  }
  return std::string(id->data.AST_IDENTIFIER.value,
                     id->data.AST_IDENTIFIER.length);
}

static bool audio_mlir_source(Ast *ast, Ast **out_lambda, Ast *out_binding) {
  if (!ast || ast->tag != AST_APPLICATION) {
    return false;
  }

  Ast *source = ast->data.AST_APPLICATION.args;
  if (!source) {
    return false;
  }

  if (source->tag == AST_LAMBDA) {
    *out_lambda = source;
    ObjString fn_name = source->data.AST_LAMBDA.fn_name;
    memset(out_binding, 0, sizeof(*out_binding));
    out_binding->tag = AST_IDENTIFIER;
    out_binding->data.AST_IDENTIFIER.value = fn_name.chars;
    out_binding->data.AST_IDENTIFIER.length = (size_t)fn_name.length;
    return true;
  }

  if (source->tag == AST_LET && source->data.AST_LET.expr &&
      source->data.AST_LET.expr->tag == AST_LAMBDA &&
      source->data.AST_LET.binding &&
      source->data.AST_LET.binding->tag == AST_IDENTIFIER) {
    *out_lambda = source->data.AST_LET.expr;
    *out_binding = *source->data.AST_LET.binding;
    return true;
  }

  return false;
}

static ylc::audio_mlir::MlirSynthNames
make_names(const std::string &public_name, int synth_id) {
  int generation = g_symbol_counter++;
  std::string prefix = "__ylc_audio_mlir." + std::to_string(synth_id) + "." +
                       std::to_string(generation);
  return {
      .public_name = public_name,
      .prefix = prefix,
      .cons = prefix + ".cons",
      .init = prefix + ".init",
      .perform = prefix + ".perform",
      .frame = prefix + ".frame",
  };
}

static int synth_id_from_symbol(JITSymbol *sym) {
  return sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
}

static void register_builtin(ht *stack, const char *name,
                             BuiltinHandler handler) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = handler;
  ht_set_hash(stack, name, hash_string(name, (int)strlen(name)), sym);
}

static LLVMValueRef declare_or_get(LLVMModuleRef module, const char *name,
                                   LLVMTypeRef fn_ty) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);
  if (!fn) {
    fn = LLVMAddFunction(module, name, fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static LLVMValueRef call_audio_mlir_symbol(Ast *ast, JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  Ast *sym_ast = ast->data.AST_APPLICATION.function;
  JITSymbol *sym = lookup_id_ast(sym_ast, ctx);
  if (!sym) {
    fprintf(stderr, "audio_jit_mlir: unresolved synth symbol\n");
    return LLVMConstNull(ptr_type(module));
  }

  int synth_id = synth_id_from_symbol(sym);
  if (synth_id < 0 || synth_id >= (int)g_synths.size() ||
      !g_synths[(size_t)synth_id]) {
    fprintf(stderr, "audio_jit_mlir: synth id out of range: %d\n", synth_id);
    return LLVMConstNull(ptr_type(module));
  }

  MlirSynthRecord *record = g_synths[(size_t)synth_id].get();
  unsigned formal_count = record->arg_count;
  std::vector<LLVMTypeRef> formal_tys(formal_count, f64_type(module));
  std::vector<LLVMValueRef> ctor_args(formal_count);

  for (unsigned i = 0; i < formal_count; i++) {
    if (i < ast->data.AST_APPLICATION.len) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + i;
      LLVMValueRef arg_val = codegen(arg_ast, ctx, module, builder);
      arg_val = handle_type_conversions(arg_val, arg_ast->type, &t_num, ctx,
                                        module, builder);
      ctor_args[i] = arg_val;
    } else {
      ctor_args[i] = LLVMConstReal(f64_type(module), 0.0);
    }
  }

  ast->type = &t_ptr;

  LLVMTypeRef ctor_ty =
      LLVMFunctionType(ptr_type(module), formal_tys.data(), formal_count, 0);
  LLVMTypeRef get_ty = LLVMFunctionType(
      ptr_type(module), (LLVMTypeRef[]){i32_type(module)}, 1, 0);
  LLVMValueRef get_fn =
      declare_or_get(module, "ylc_audio_mlir_get_synth_ctor", get_ty);
  LLVMValueRef synth_id_val =
      LLVMConstInt(i32_type(module), (unsigned long long)synth_id, 0);
  LLVMValueRef ctor_ptr =
      LLVMBuildCall2(builder, get_ty, get_fn, &synth_id_val, 1, "ctor.ptr");
  return LLVMBuildCall2(builder, ctor_ty, ctor_ptr, ctor_args.data(),
                        formal_count, "audio.mlir.node");
}

} // namespace

extern "C" void ylc_audio_mlir_register_synth_ctor(int synth_id, void *ctor) {
  if (synth_id < 0 || synth_id >= (int)g_synths.size() ||
      !g_synths[(size_t)synth_id]) {
    return;
  }
  g_synths[(size_t)synth_id]->ctor_ptr.store(ctor, std::memory_order_release);
}

extern "C" void *ylc_audio_mlir_get_synth_ctor(int synth_id) {
  if (synth_id < 0 || synth_id >= (int)g_synths.size() ||
      !g_synths[(size_t)synth_id]) {
    return nullptr;
  }
  return g_synths[(size_t)synth_id]->ctor_ptr.load(std::memory_order_acquire);
}

extern "C" void *ylc_audio_mlir_create_audio_node(void *perform, int num_inputs,
                                                  int output_layout,
                                                  int state_bytes,
                                                  const char *meta_name) {
  size_t state_size = state_bytes > 0 ? (size_t)state_bytes : 0;
  size_t lanes = output_layout > 0 ? (size_t)output_layout : 1;
  size_t total =
      sizeof(Node) + state_size + (size_t)BUF_SIZE * lanes * sizeof(double);
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return nullptr;
  }

  node->perform = (perform_func_t)perform;
  node->num_inputs = num_inputs;
  node->state_size = (int)state_size;
  node->state_ptr = (char *)node + sizeof(Node);
  node->meta = (char *)(meta_name ? meta_name : "audio_mlir");
  node->output = (Signal){
      .layout = (int)lanes,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_size),
  };
  return node;
}

extern "C" void *ylc_audio_mlir_node_state(void *node_raw) {
  Node *node = (Node *)node_raw;
  if (!node) {
    return nullptr;
  }
  if (node->state_ptr) {
    return node->state_ptr;
  }
  return (char *)node + sizeof(Node);
}

extern "C" void *ylc_audio_mlir_get_output_buf(void *node_raw) {
  Node *node = (Node *)node_raw;
  return node ? node->output.buf : nullptr;
}

extern "C" double ylc_audio_mlir_read_inlet_node(void *node_raw,
                                                 int64_t frame) {
  Node *node = (Node *)node_raw;
  if (!node || !node->output.buf || node->output.size <= 0) {
    return 0.0;
  }
  int64_t idx = frame;
  if (idx < 0) {
    idx = 0;
  }
  if (idx >= node->output.size) {
    idx %= node->output.size;
  }
  return node->output.buf[idx * node->output.layout];
}

extern "C" void *ylc_audio_mlir_const_inlet(double val) {
  AudioGraph *graph = _graph;
  Node *node = nullptr;

  if (graph) {
    node = allocate_node_in_graph(graph, 0);
    int saved_idx = node->node_index;
    memset(node, 0, sizeof(*node));
    node->node_index = saved_idx;
    node->num_inputs = 0;
    node->state_size = 0;
    node->state_offset = graph->state_memory_size;
    node->output = (Signal){.layout = 1,
                            .size = BUF_SIZE,
                            .buf = allocate_buffer_from_pool(graph, BUF_SIZE)};
    node->meta = (char *)"audio_mlir_const_inlet";
  } else {
    size_t total = sizeof(Node) + (size_t)BUF_SIZE * sizeof(double);
    node = (Node *)calloc(1, total);
    if (!node) {
      return nullptr;
    }
    node->output = (Signal){
        .layout = 1,
        .size = BUF_SIZE,
        .buf = (double *)((char *)node + sizeof(Node)),
    };
    node->meta = (char *)"audio_mlir_const_inlet";
  }

  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.buf[i] = val;
  }
  return node;
}

extern "C" LLVMValueRef CompileAudioMLIRFnHandler(Ast *ast, JITLangCtx *ctx,
                                                  LLVMModuleRef module,
                                                  LLVMBuilderRef builder) {
  Ast *lambda = nullptr;
  Ast binding = {};
  if (!audio_mlir_source(ast, &lambda, &binding)) {
    fprintf(stderr, "audio_jit_mlir: expected Audio(lambda) or "
                    "Audio(let name = lambda)\n");
    return LLVMConstNull(ptr_type(module));
  }

  std::string public_name = ast_identifier_string(&binding);
  JITSymbol *existing = lookup_id_ast(&binding, ctx);
  int synth_id;
  void *old_ctor_ptr = nullptr;
  if (existing && existing->type == (symbol_type)STYPE_AUDIO_JIT_MLIR_SYM) {
    synth_id = synth_id_from_symbol(existing);
    if (synth_id >= 0 && synth_id < (int)g_synths.size() &&
        g_synths[(size_t)synth_id]) {
      old_ctor_ptr =
          g_synths[(size_t)synth_id]->ctor_ptr.load(std::memory_order_acquire);
    }
  } else {
    synth_id = (int)g_synths.size();
  }

  ylc::audio_mlir::MlirSynthNames names = make_names(public_name, synth_id);
  ylc::audio_mlir::MlirSynthCompileResult compiled =
      ylc::audio_mlir::compile_mlir_synth_stub(lambda, names, ctx, module,
                                               builder);
  if (!compiled.ok) {
    return LLVMConstNull(ptr_type(module));
  }

  LLVMTypeRef reg_ty = LLVMFunctionType(
      void_type(module), (LLVMTypeRef[]){i32_type(module), ptr_type(module)}, 2,
      0);
  LLVMValueRef reg_fn =
      declare_or_get(module, "ylc_audio_mlir_register_synth_ctor", reg_ty);
  LLVMValueRef reg_args[] = {
      LLVMConstInt(i32_type(module), (unsigned long long)synth_id, 0),
      compiled.cons_fn,
  };
  LLVMBuildCall2(builder, reg_ty, reg_fn, reg_args, 2, "");

  auto record = std::make_unique<MlirSynthRecord>();
  record->public_name = public_name;
  record->names = compiled.names;
  record->cons_fn = compiled.cons_fn;
  record->init_fn = compiled.init_fn;
  record->perform_fn = compiled.perform_fn;
  record->frame_fn = compiled.frame_fn;
  record->arg_count = compiled.arg_count;
  record->output_lanes = compiled.output_lanes;
  record->state_bytes = compiled.state_bytes;
  record->ctor_ptr.store(old_ctor_ptr, std::memory_order_release);

  if (synth_id == (int)g_synths.size()) {
    g_synths.push_back(std::move(record));
  } else {
    g_synths[(size_t)synth_id] = std::move(record);
  }

  if (!(existing && existing->type == (symbol_type)STYPE_AUDIO_JIT_MLIR_SYM)) {
    JITSymbol *sym =
        new_symbol((symbol_type)STYPE_AUDIO_JIT_MLIR_SYM, NULL, NULL, NULL);
    sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler =
        call_audio_mlir_symbol;
    sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = synth_id;
    ht_set_hash(ctx->frame->table, public_name.c_str(),
                hash_string(public_name.c_str(), (int)public_name.size()), sym);
  }

  fprintf(stderr, "audio_jit_mlir: compiled stub synth %s as %s\n",
          public_name.c_str(), names.prefix.c_str());
  return compiled.cons_fn;
}

__attribute__((constructor)) static void ylc_audio_jit_mlir_init(void) {
  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit_mlir: no JIT context at load time\n");
    return;
  }

  STYPE_AUDIO_JIT_MLIR_SYM = REGISTERED_JIT_SYMBOL_TYPE++;

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "Audio", CompileAudioMLIRFnHandler);

  fprintf(stderr, "libaudio_jit_mlir: registered Audio\n");
}
