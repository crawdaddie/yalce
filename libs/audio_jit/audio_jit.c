#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/ylc_datatypes.h"
#include "./osc_kernels.h"
#include "mir/mir.h"
#include "serde.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *ylc_get_output_buf(void *node_raw) {
  return ((Node *)node_raw)->output.buf;
}

void *ylc_audio_node_inline_state(void *node_raw) {
  Node *node = (Node *)node_raw;
  return node ? (node->state_ptr ? node->state_ptr : (void *)(node + 1)) : NULL;
}

void ylc_audio_memzero(void *ptr, int32_t size) {
  if (ptr && size > 0) {
    memset(ptr, 0, (size_t)size);
  }
}

extern double ylc_read_inlet_node(void *node_raw, int64_t frame);
double ylc_read_inlet_node_i32(void *node_raw, int frame) {
  return ylc_read_inlet_node(node_raw, (int64_t)frame);
}

Node *ylc_create_audio_frame_node(frame_perform_func_t frame_perform,
                                  int num_inputs, int output_layout,
                                  int state_bytes, const char *meta_name) {
  size_t total = sizeof(Node) + (size_t)state_bytes +
                 ((size_t)BUF_SIZE * (size_t)output_layout * sizeof(double));
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return NULL;
  }

  node->frame_perform = frame_perform;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
  node->state_ptr = state_bytes ? (void *)((char *)node + sizeof(Node)) : NULL;
  node->meta = (char *)meta_name;
  node->output = (Signal){
      .layout = output_layout,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->next = NULL;

  return node;
}

#define MIR_AUDIO_CONTEXT_KIND "Audio"

typedef struct MirAudioSynthBuildCtx {
  MirProgram *program;
  MirArena *arena;
  Ast *app;
  Ast *lambda;
  MirCtx *parent_ctx;
  const char *name;
  int num_inputs;
  Ast **capture_asts;
  size_t capture_count;
  int state_bytes;

  MirFunction *ctor_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;

  MirBuilder ctor_builder;
  MirBuilder init_builder;
  MirBuilder kernel_builder;
  MirBuilder frame_builder;
} MirAudioSynthBuildCtx;

typedef enum AudioSynthScope {
  AUDIO_SYNTH_EXPORTED,
  AUDIO_SYNTH_LOCAL,
} AudioSynthScope;

typedef struct MirAudioSynthSymbol {
  const char *name;
  AudioSynthScope scope;
  int num_inputs;
  Ast **capture_asts;
  size_t capture_count;
  int state_bytes;
  MirFunction *ctor_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;
} MirAudioSynthSymbol;

static MirValueId
audio_mir_call_synth_in_audio_context(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirAudioSynthSymbol *synth);
static MirValueId MirAudioSynthSymbolHandler(MirBuilder *builder, Ast *app,
                                             MirCtx *ctx, MirSymbol *symbol);

typedef struct AudioValue AudioValue;

typedef enum AudioValueKind {
  AUDIO_VALUE_NONE,
  AUDIO_VALUE_MIR,
  AUDIO_VALUE_SYNTH,
  AUDIO_VALUE_PARTIAL_SYNTH,
  AUDIO_VALUE_PARTIAL_BUILTIN,
} AudioValueKind;

typedef struct AudioPartialBuiltin {
  const char *name;
  Type *type;
  size_t argc;
  AudioValue *args;
} AudioPartialBuiltin;

typedef struct AudioPartialSynth {
  MirAudioSynthSymbol *synth;
  Type *type;
  size_t argc;
  AudioValue *args;
} AudioPartialSynth;

struct AudioValue {
  AudioValueKind kind;
  Type *type;
  MirValueId value;
  int lanes;
  MirValueId *vec;
  MirAudioSynthSymbol *synth;
  AudioPartialSynth *partial_synth;
  AudioPartialBuiltin *partial;
};

typedef struct AudioStateSlot {
  size_t offset;
  size_t size;
  size_t align;
  Type *type;
  const char *name;
  bool zero_bytes;
  struct AudioStateSlot *next;
} AudioStateSlot;

typedef struct AudioStateInitStore {
  size_t offset;
  Type *type;
  Ast *expr;
  struct AudioStateInitStore *next;
} AudioStateInitStore;

typedef struct AudioInitCall {
  MirFunction *init_fn;
  size_t state_offset;
  Type *state_type;
  const char *name;
  struct AudioInitCall *next;
} AudioInitCall;

typedef struct AudioFnRefCache {
  MirFunction *owner;
  MirBlock *block;
  MirFunction *target;
  Type *type;
  MirValueId value;
  struct AudioFnRefCache *next;
} AudioFnRefCache;

typedef struct AudioLocalBinding {
  const char *name;
  AudioValue value;
  struct AudioLocalBinding *next;
} AudioLocalBinding;

typedef struct AudioCompileCtx {
  MirAudioSynthBuildCtx *bundle;
  MirProgram *program;
  MirArena *arena;
  Ast *app;
  Ast *lambda;
  MirCtx *parent_ctx;
  MirCtx *mir_ctx;

  MirBuilder *cons_builder;
  MirBuilder *init_builder;
  MirBuilder *kernel_builder;
  MirBuilder *frame_builder;

  MirFunction *cons_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;
  MirFunction *bundle_fns[4];
  size_t bundle_fns_len;

  Type *ptr_char_type;
  Type *ptr_double_type;
  Type *ptr_ptr_type;

  MirValueId node_param;
  MirValueId state_param;
  MirValueId frame_param;
  MirValueId spf_param;
  MirValueId inputs_param;

  size_t state_cursor;
  AudioStateSlot *state_slots;
  AudioStateInitStore *state_inits;
  AudioInitCall *init_calls;
  AudioFnRefCache *fn_refs;
  AudioLocalBinding *locals;
} AudioCompileCtx;

static AudioValue audio_mir_apply_audio_value(AudioCompileCtx *audio,
                                              Ast *origin, AudioValue callable,
                                              AudioValue *args,
                                              size_t arg_count,
                                              const char *instance_prefix);
static AudioValue audio_mir_emit_audio_hof(AudioCompileCtx *audio, Ast *app,
                                           const char *name);

typedef struct AudioBundleOptCtx {
  MirProgram *program;
  MirArena *arena;
  MirFunction *cons_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;
  MirFunction **fns;
  size_t fns_len;
} AudioBundleOptCtx;

typedef struct AudioMirKernelArgLanes {
  Type *type;
  MirValueId *values;
} AudioMirKernelArgLanes;

typedef struct AudioBuiltin AudioBuiltin;
typedef AudioValue (*AudioBuiltinEmitFn)(const AudioBuiltin *builtin,
                                         AudioCompileCtx *audio, Ast *origin,
                                         AudioValue *args, size_t argc);

struct AudioBuiltin {
  const char *name;
  size_t source_argc;
  const char *kernel_symbol;
  AudioBuiltinEmitFn emit;
  size_t state_size;
  size_t state_align;
  const char *state_name;
  uint64_t lane_expand_mask;
  const size_t *arg_order;
  size_t kernel_argc;
};

#define AUDIO_ARG_MASK(index) (UINT64_C(1) << (index))
#define AUDIO_ARG_MASK_ALL(argc)                                               \
  ((argc) >= 64 ? UINT64_MAX : ((UINT64_C(1) << (argc)) - UINT64_C(1)))

static uint64_t audio_arg_mask(size_t index) {
  return index < 64 ? (UINT64_C(1) << index) : 0;
}

static MirValueId MirCompileAudioNodeBuiltinHandler(MirBuilder *builder,
                                                    Ast *app, MirCtx *ctx,
                                                    MirBuiltinSymbol *symbol);

#define AUDIO_VALUE_NULL                                                       \
  (AudioValue) {                                                               \
    .kind = AUDIO_VALUE_NONE, .type = NULL, .value = MIR_NO_VALUE, .lanes = 0, \
    .vec = NULL, .synth = NULL, .partial_synth = NULL, .partial = NULL         \
  }

static MirArena *audio_mir_bundle_arena(MirBuilder *builder) {
  if (!builder || !builder->program) {
    return NULL;
  }
  return builder->program->durable_arena ? builder->program->durable_arena
                                         : builder->program->arena;
}

static Type *audio_mir_fn_type(MirArena *arena, Type **params,
                               size_t params_len, Type *ret) {
  if (!arena || !ret || (params_len && !params)) {
    return NULL;
  }
  if (params_len == 0) {
    Type *fn = mir_arena_alloc(arena, sizeof(Type), __alignof__(Type));
    if (!fn) {
      return NULL;
    }
    memset(fn, 0, sizeof(*fn));
    fn->kind = T_FN;
    fn->data.T_FN.from = &t_void;
    fn->data.T_FN.to = ret;
    return fn;
  }

  Type *type = ret;
  for (size_t i = params_len; i > 0; i--) {
    Type *fn = mir_arena_alloc(arena, sizeof(Type), __alignof__(Type));
    if (!fn) {
      return NULL;
    }
    memset(fn, 0, sizeof(*fn));
    fn->kind = T_FN;
    fn->data.T_FN.from = params[i - 1];
    fn->data.T_FN.to = type;
    type = fn;
  }
  return type;
}

static Type *audio_mir_ptr_to(MirArena *arena, Type *pointee) {
  if (!arena || !pointee) {
    return &t_ptr;
  }

  Type **args = mir_arena_alloc(arena, sizeof(Type *), __alignof__(Type *));
  Type *type = mir_arena_alloc(arena, sizeof(Type), __alignof__(Type));
  if (!args || !type) {
    return &t_ptr;
  }
  args[0] = pointee;
  memset(type, 0, sizeof(*type));
  type->kind = T_CONS;
  type->data.T_CONS.name = TYPE_NAME_PTR;
  type->data.T_CONS.args = args;
  type->data.T_CONS.num_args = 1;
  type->alias = TYPE_NAME_PTR;
  return type;
}

static size_t audio_mir_align_up(size_t value, size_t align) {
  if (align == 0) {
    return value;
  }
  return (value + align - 1) & ~(align - 1);
}

static int audio_mir_sizeof_type(Type *type) {
  if (!type) {
    return (int)sizeof(void *);
  }
  switch (type->kind) {
  case T_BOOL:
  case T_CHAR:
    return 1;
  case T_INT:
    return 4;
  case T_NUM:
  case T_UINT64:
    return 8;
  default:
    return (int)sizeof(void *);
  }
}

static MirFunction *audio_mir_extern_fn(MirBuilder *builder, const char *name,
                                        Type *type, Ast *origin) {
  if (!builder || !builder->program || !name || !type) {
    return NULL;
  }
  return mir_program_add_extern_function(builder->program, name, type, origin);
}

static MirValueId audio_mir_extern_ref(MirBuilder *builder, const char *name,
                                       Type *type, Ast *origin) {
  MirFunction *fn = audio_mir_extern_fn(builder, name, type, origin);
  return fn ? mir_fn_ref(builder, type, origin, fn) : MIR_NO_VALUE;
}

static MirValueId audio_mir_fn_ref(AudioCompileCtx *audio, MirBuilder *builder,
                                   Type *type, Ast *origin, MirFunction *fn) {
  if (!builder || !fn) {
    return MIR_NO_VALUE;
  }

  Type *ref_type = type ? type : fn->type;
  if (!audio || !audio->arena || !builder->fn || !builder->block) {
    return mir_fn_ref(builder, ref_type, origin, fn);
  }

  for (AudioFnRefCache *ref = audio->fn_refs; ref; ref = ref->next) {
    if (ref->owner == builder->fn && ref->block == builder->block &&
        ref->target == fn &&
        (ref->type == ref_type || types_equal(ref->type, ref_type))) {
      return ref->value;
    }
  }

  MirValueId value = mir_fn_ref(builder, ref_type, origin, fn);
  if (value == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  AudioFnRefCache *ref = mir_arena_alloc(audio->arena, sizeof(AudioFnRefCache),
                                         __alignof__(AudioFnRefCache));
  if (!ref) {
    return value;
  }
  *ref = (AudioFnRefCache){
      .owner = builder->fn,
      .block = builder->block,
      .target = fn,
      .type = ref_type,
      .value = value,
      .next = audio->fn_refs,
  };
  audio->fn_refs = ref;
  return value;
}

static bool audio_mir_add_params(MirFunction *fn, Type **params,
                                 const char **names, Ast **origins,
                                 size_t params_len, Ast *origin) {
  if (!fn || (params_len && !params)) {
    return false;
  }
  for (size_t i = 0; i < params_len; i++) {
    const char *name =
        names && names[i] ? names[i] : mir_arena_printf(fn->arena, "arg%zu", i);
    Ast *param_origin = origins && origins[i] ? origins[i] : origin;
    if (mir_function_add_param(fn, name ? name : "arg", params[i],
                               param_origin) == MIR_NO_VALUE) {
      return false;
    }
  }
  return true;
}

static AudioValue audio_mir_expr(AudioCompileCtx *audio, Ast *ast);
static AudioValue
audio_mir_emit_synth_in_audio_context(AudioCompileCtx *audio, Ast *app,
                                      MirAudioSynthSymbol *synth);
static void audio_mir_emit_local_synth_stubs(MirAudioSynthBuildCtx *ctx);

static MirFunction *audio_mir_build_bundle_fn(MirAudioSynthBuildCtx *ctx,
                                              const char *suffix, Type *type,
                                              Type **params, const char **names,
                                              Ast **origins, size_t params_len,
                                              MirBuilder *out_builder) {
  if (!ctx || !ctx->program || !suffix || !type || !out_builder) {
    return NULL;
  }

  const char *name =
      mir_arena_printf(ctx->program->arena, "%s.%s",
                       ctx->name ? ctx->name : "audio_synth", suffix);
  MirFunction *fn = mir_program_add_function_arena(
      ctx->program, name, type, ctx->app,
      ctx->arena ? ctx->arena : ctx->program->arena);
  if (fn) {
    fn->skip_rc_markers = true;
  }
  if (!fn ||
      !audio_mir_add_params(fn, params, names, origins, params_len, ctx->app)) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(fn, "entry");
  if (!entry) {
    return NULL;
  }

  mir_builder_init(out_builder, ctx->program, fn);
  mir_builder_position_at_end(out_builder, entry);

  return fn;
}

static MirFunction *audio_mir_build_frame_fn(MirAudioSynthBuildCtx *ctx) {
  if (!ctx || !ctx->program || !ctx->program->arena) {
    return NULL;
  }

  MirArena *arena = ctx->arena ? ctx->arena : ctx->program->arena;
  Type *ptr_char = audio_mir_ptr_to(arena, &t_char);
  Type *ptr_ptr = audio_mir_ptr_to(arena, &t_ptr);
  Type *params[] = {&t_ptr, ptr_char, ptr_ptr, &t_int, &t_num};
  const char *names[] = {"node", "state", "inputs", "frame", "spf"};
  size_t params_len = sizeof(params) / sizeof(params[0]);
  Type *type = audio_mir_fn_type(arena, params, params_len, &t_void);
  if (!type) {
    return NULL;
  }

  const char *name = mir_arena_printf(ctx->program->arena, "%s.frame",
                                      ctx->name ? ctx->name : "audio_synth");
  MirFunction *fn = mir_program_add_function_arena(
      ctx->program, name, type, ctx->app,
      ctx->arena ? ctx->arena : ctx->program->arena);
  if (fn) {
    fn->skip_rc_markers = true;
  }
  if (!fn ||
      !audio_mir_add_params(fn, params, names, NULL, params_len, ctx->app)) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(fn, "entry");
  if (!entry) {
    return NULL;
  }

  mir_builder_init(&ctx->frame_builder, ctx->program, fn);
  mir_builder_position_at_end(&ctx->frame_builder, entry);
  return fn;
}

static Type *audio_mir_kernel_return_type(MirAudioSynthBuildCtx *ctx) {
  if (!ctx || !ctx->lambda || ctx->lambda->tag != AST_LAMBDA) {
    return &t_num;
  }
  Ast *body = ctx->lambda->data.AST_LAMBDA.body;
  if (body && body->type) {
    return body->type;
  }
  if (ctx->lambda->type) {
    Type *ret = fn_return_type(ctx->lambda->type);
    if (ret) {
      return ret;
    }
  }
  return &t_num;
}

static bool audio_mir_set_fn_return_type(MirArena *arena, MirFunction *fn,
                                         Type *return_type) {
  if (!arena || !fn || !return_type) {
    return false;
  }

  size_t params_len = fn->params.len;
  Type **params = params_len
                      ? mir_arena_alloc(arena, sizeof(Type *) * params_len,
                                        __alignof__(Type *))
                      : NULL;
  if (params_len && !params) {
    return false;
  }

  for (size_t i = 0; i < params_len; i++) {
    params[i] = fn->params.items[i].type ? fn->params.items[i].type : &t_void;
  }

  Type *type = audio_mir_fn_type(arena, params, params_len, return_type);
  if (!type) {
    return false;
  }
  fn->type = type;
  return true;
}

static bool audio_mir_is_tuple_type(Type *type) {
  return type && type->kind == T_CONS && type->data.T_CONS.name &&
         strcmp(type->data.T_CONS.name, TYPE_NAME_TUPLE) == 0;
}

static int audio_mir_type_lanes(Type *type) {
  if (audio_mir_is_tuple_type(type) && type->data.T_CONS.num_args > 0) {
    return type->data.T_CONS.num_args;
  }
  return type && type->kind != T_VOID ? 1 : 0;
}

static int audio_mir_kernel_output_lanes(MirFunction *kernel_fn) {
  Type *return_type = kernel_fn ? fn_return_type(kernel_fn->type) : NULL;
  int lanes = audio_mir_type_lanes(return_type);
  return lanes > 0 ? lanes : 1;
}

static MirFunction *audio_mir_build_kernel_fn(MirAudioSynthBuildCtx *ctx,
                                              Type **input_params,
                                              const char **input_names,
                                              Ast **input_origins,
                                              size_t input_params_len) {
  if (!ctx || !ctx->program || !ctx->program->arena ||
      (input_params_len && (!input_params || !input_names))) {
    return NULL;
  }

  MirArena *arena = ctx->arena ? ctx->arena : ctx->program->arena;
  size_t params_len = input_params_len + 4;
  Type **params = params_len
                      ? mir_arena_alloc(arena, sizeof(Type *) * params_len,
                                        __alignof__(Type *))
                      : NULL;
  const char **names = params_len
                           ? mir_arena_alloc(arena, sizeof(char *) * params_len,
                                             __alignof__(char *))
                           : NULL;
  Ast **origins = params_len
                      ? mir_arena_alloc(arena, sizeof(Ast *) * params_len,
                                        __alignof__(Ast *))
                      : NULL;
  if (!params || !names || !origins) {
    return NULL;
  }

  params[0] = &t_ptr;
  names[0] = "node";
  origins[0] = ctx->app;
  params[1] = audio_mir_ptr_to(arena, &t_char);
  names[1] = "state";
  origins[1] = ctx->app;
  params[2] = &t_int;
  names[2] = "frame";
  origins[2] = ctx->app;
  params[3] = &t_num;
  names[3] = "spf";
  origins[3] = ctx->app;
  for (size_t i = 0; i < input_params_len; i++) {
    params[i + 4] = input_params[i] ? input_params[i] : &t_num;
    names[i + 4] = input_names[i] ? input_names[i] : "arg";
    origins[i + 4] =
        input_origins && input_origins[i] ? input_origins[i] : ctx->app;
  }

  Type *type = audio_mir_fn_type(arena, params, params_len,
                                 audio_mir_kernel_return_type(ctx));
  if (!type) {
    return NULL;
  }

  return audio_mir_build_bundle_fn(ctx, "kernel", type, params, names, origins,
                                   params_len, &ctx->kernel_builder);
}

static bool audio_mir_bind_value_name(MirCtx *mir_ctx, const char *name,
                                      MirValueId value) {
  return !name || value == MIR_NO_VALUE ||
         mir_ctx_bind_value(mir_ctx, name, value);
}

static bool audio_mir_bind_frame_abi_params(MirAudioSynthBuildCtx *ctx,
                                            MirCtx *mir_ctx) {
  if (!ctx || !ctx->frame_fn || !mir_ctx || ctx->frame_fn->params.len < 5) {
    return false;
  }

  for (size_t i = 0; i < ctx->frame_fn->params.len; i++) {
    MirParam *param = &ctx->frame_fn->params.items[i];
    if (!audio_mir_bind_value_name(mir_ctx, param->name, param->value)) {
      return false;
    }
  }
  return true;
}

static bool audio_mir_bind_kernel_lambda_param(MirAudioSynthBuildCtx *ctx,
                                               MirCtx *mir_ctx, Ast *pattern,
                                               Type *type,
                                               size_t *kernel_param_index) {
  if (!ctx || !ctx->kernel_fn || !mir_ctx || !pattern || !kernel_param_index) {
    return false;
  }

  switch (pattern->tag) {
  case AST_PLACEHOLDER_ID:
  case AST_VOID:
    return true;
  case AST_IDENTIFIER: {
    (void)type;
    if (*kernel_param_index >= ctx->kernel_fn->params.len) {
      return false;
    }
    MirParam *param = &ctx->kernel_fn->params.items[*kernel_param_index];
    if (param->value == MIR_NO_VALUE ||
        !mir_ctx_bind_value(mir_ctx, pattern->data.AST_IDENTIFIER.value,
                            param->value)) {
      return false;
    }
    (*kernel_param_index)++;
    return true;
  }
  case AST_TUPLE:
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      Ast *item = pattern->data.AST_LIST.items + i;
      Type *item_type = item->type;
      if (type && type->kind == T_CONS && type->data.T_CONS.args &&
          i < (size_t)type->data.T_CONS.num_args) {
        item_type = type->data.T_CONS.args[i];
      }
      if (!audio_mir_bind_kernel_lambda_param(ctx, mir_ctx, item, item_type,
                                              kernel_param_index)) {
        return false;
      }
    }
    return true;
  default:
    return false;
  }
}

static bool audio_mir_bind_kernel_lambda_params(MirAudioSynthBuildCtx *ctx,
                                                MirCtx *mir_ctx) {
  if (!ctx || !ctx->kernel_fn || !ctx->lambda ||
      ctx->lambda->tag != AST_LAMBDA || !mir_ctx) {
    return false;
  }

  if (ctx->lambda->type && is_void_func(ctx->lambda->type)) {
    return true;
  }

  Type *fn_type = ctx->lambda->type;
  size_t kernel_param_index = 4 + ctx->capture_count;
  for (AstList *p = ctx->lambda->data.AST_LAMBDA.params; p; p = p->next) {
    Type *param_type = p->ast ? p->ast->type : &t_num;
    if (fn_type && fn_type->kind == T_FN) {
      param_type = fn_type->data.T_FN.from;
      fn_type = fn_type->data.T_FN.to;
    }
    if (!audio_mir_bind_kernel_lambda_param(ctx, mir_ctx, p->ast, param_type,
                                            &kernel_param_index)) {
      return false;
    }
  }
  return true;
}

static MirValueId audio_mir_kernel_body_value(MirAudioSynthBuildCtx *ctx) {
  if (!ctx || !ctx->kernel_fn || !ctx->lambda ||
      ctx->lambda->tag != AST_LAMBDA || !ctx->lambda->data.AST_LAMBDA.body) {
    return MIR_NO_VALUE;
  }

  MirCtx frame_ctx = {
      .env = ctx->parent_ctx && ctx->parent_ctx->env ? ctx->parent_ctx->env
                                                     : ctx->program->type_env,
      .frame = NULL,
      .current_module = ctx->parent_ctx ? ctx->parent_ctx->current_module
                                        : ctx->program->root_module,
      .export_bindings = false,
      .prefer_global_loads = true,
      .extension_kind = MIR_AUDIO_CONTEXT_KIND,
      .extension_data = ctx,
  };
  ht frame_table;
  MirStackFrame frame;
  MirStackFrame *parent = ctx->parent_ctx ? ctx->parent_ctx->frame : NULL;
  mir_stack_frame_init(ctx->kernel_fn->arena, &frame_table, &frame, parent);
  frame_ctx.frame = &frame;

  for (size_t i = 0; i < ctx->kernel_fn->params.len; i++) {
    MirParam *param = &ctx->kernel_fn->params.items[i];
    if (!audio_mir_bind_value_name(&frame_ctx, param->name, param->value)) {
      return MIR_NO_VALUE;
    }
  }
  if (!audio_mir_bind_kernel_lambda_params(ctx, &frame_ctx)) {
    return MIR_NO_VALUE;
  }

  return mir_expr(&ctx->kernel_builder, ctx->lambda->data.AST_LAMBDA.body,
                  &frame_ctx);
}

static int audio_mir_lambda_input_count(Ast *lambda) {
  if (!lambda || lambda->tag != AST_LAMBDA || !lambda->type ||
      is_void_func(lambda->type)) {
    return 0;
  }

  int count = 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast && p->ast->tag == AST_TUPLE) {
      count += p->ast->data.AST_LIST.len;
    } else if (p->ast && p->ast->tag == AST_IDENTIFIER) {
      count++;
    }
  }
  return count;
}

static const char *audio_mir_lambda_name(MirArena *arena, Ast *lambda,
                                         Ast *source) {
  if (!arena) {
    return "audio_synth";
  }
  if (source && source->tag == AST_LET && source->data.AST_LET.binding &&
      source->data.AST_LET.binding->tag == AST_IDENTIFIER) {
    return mir_arena_strdup(
        arena, source->data.AST_LET.binding->data.AST_IDENTIFIER.value);
  }
  if (lambda && lambda->tag == AST_LAMBDA &&
      lambda->data.AST_LAMBDA.fn_name.chars &&
      lambda->data.AST_LAMBDA.fn_name.length > 0) {
    return mir_arena_strndup(arena, lambda->data.AST_LAMBDA.fn_name.chars,
                             lambda->data.AST_LAMBDA.fn_name.length);
  }
  return mir_arena_strdup(arena, "audio_synth");
}

static Ast *audio_mir_source_lambda(Ast *source) {
  if (!source) {
    return NULL;
  }
  if (source->tag == AST_LAMBDA) {
    return source;
  }
  if (source->tag == AST_LET && source->data.AST_LET.expr &&
      source->data.AST_LET.expr->tag == AST_LAMBDA) {
    return source->data.AST_LET.expr;
  }
  return NULL;
}

static const char *audio_mir_param_name(MirArena *arena, Ast *param,
                                        size_t index) {
  if (param && param->tag == AST_IDENTIFIER &&
      param->data.AST_IDENTIFIER.value) {
    return param->data.AST_IDENTIFIER.value;
  }
  return mir_arena_printf(arena, "arg%zu", index);
}

static bool audio_mir_collect_ctor_params(MirAudioSynthBuildCtx *ctx,
                                          Type **params, const char **names,
                                          Ast **origins, size_t params_len) {
  if (!ctx || !ctx->lambda || ctx->lambda->tag != AST_LAMBDA ||
      (params_len && (!params || !names || !origins))) {
    return false;
  }

  MirArena *arena = ctx->arena ? ctx->arena : ctx->program->arena;
  Type *fn_type = ctx->lambda->type;
  size_t index = 0;

  if (ctx->lambda->type && is_void_func(ctx->lambda->type)) {
    return params_len == 0;
  }

  for (AstList *p = ctx->lambda->data.AST_LAMBDA.params;
       p && index < params_len; p = p->next) {
    Ast *param_ast = p->ast;
    Type *param_type = NULL;
    if (fn_type && fn_type->kind == T_FN) {
      param_type = fn_type->data.T_FN.from;
      fn_type = fn_type->data.T_FN.to;
    }
    if (!param_type && param_ast) {
      param_type = param_ast->type;
    }

    if (param_ast && param_ast->tag == AST_TUPLE) {
      for (size_t j = 0; j < param_ast->data.AST_LIST.len && index < params_len;
           j++) {
        Ast *field_ast = param_ast->data.AST_LIST.items + j;
        Type *field_type = NULL;
        if (param_type && param_type->kind == T_CONS &&
            param_type->data.T_CONS.args &&
            j < (size_t)param_type->data.T_CONS.num_args) {
          field_type = param_type->data.T_CONS.args[j];
        }
        if (!field_type && field_ast) {
          field_type = field_ast->type;
        }
        params[index] = field_type ? field_type : &t_num;
        names[index] = audio_mir_param_name(arena, field_ast, index);
        origins[index] = field_ast;
        index++;
      }
      continue;
    }

    params[index] = param_type ? param_type : &t_num;
    names[index] = audio_mir_param_name(arena, param_ast, index);
    origins[index] = param_ast;
    index++;
  }

  return index == params_len;
}

static Ast **audio_mir_collect_lambda_captures(MirArena *arena, Ast *lambda,
                                               size_t *out_count) {
  if (out_count) {
    *out_count = 0;
  }
  if (!arena || !lambda || lambda->tag != AST_LAMBDA ||
      lambda->data.AST_LAMBDA.num_closed_vals <= 0) {
    return NULL;
  }

  size_t count = (size_t)lambda->data.AST_LAMBDA.num_closed_vals;
  Ast **captures =
      mir_arena_alloc(arena, sizeof(Ast *) * count, __alignof__(Ast *));
  if (!captures) {
    return NULL;
  }

  size_t i = 0;
  for (AstList *cv = lambda->data.AST_LAMBDA.closed_vals; cv && i < count;
       cv = cv->next) {
    captures[i++] = cv->ast;
  }

  if (out_count) {
    *out_count = i;
  }
  return captures;
}

static const char *audio_mir_capture_param_name(MirArena *arena, Ast *capture,
                                                size_t index) {
  if (capture && capture->tag == AST_IDENTIFIER &&
      capture->data.AST_IDENTIFIER.value) {
    return capture->data.AST_IDENTIFIER.value;
  }
  return mir_arena_printf(arena, "capture%zu", index);
}

static bool audio_mir_build_synth_functions(MirAudioSynthBuildCtx *ctx) {
  if (!ctx || !ctx->program || !ctx->program->arena) {
    return false;
  }

  MirArena *arena = ctx->arena ? ctx->arena : ctx->program->arena;
  size_t num_inputs = (size_t)ctx->num_inputs;

  Type **ctor_params = num_inputs
                           ? mir_arena_alloc(arena, sizeof(Type *) * num_inputs,
                                             __alignof__(Type *))
                           : NULL;
  const char **ctor_param_names =
      num_inputs ? mir_arena_alloc(arena, sizeof(char *) * num_inputs,
                                   __alignof__(char *))
                 : NULL;
  Ast **ctor_param_origins =
      num_inputs ? mir_arena_alloc(arena, sizeof(Ast *) * num_inputs,
                                   __alignof__(Ast *))
                 : NULL;

  if (num_inputs &&
      (!ctor_params || !ctor_param_names || !ctor_param_origins)) {
    return false;
  }
  if (!audio_mir_collect_ctor_params(ctx, ctor_params, ctor_param_names,
                                     ctor_param_origins, num_inputs)) {
    return false;
  }

  Type *init_params[] = {audio_mir_ptr_to(arena, &t_char)};
  const char *init_param_names[] = {"state"};

  Type *ctor_type = audio_mir_fn_type(arena, ctor_params, num_inputs, &t_ptr);
  Type *init_type =
      audio_mir_fn_type(arena, init_params,
                        sizeof(init_params) / sizeof(init_params[0]), &t_void);
  if (!ctor_type || !init_type) {
    return false;
  }

  ctx->ctor_fn = audio_mir_build_bundle_fn(ctx, "cons", ctor_type, ctor_params,
                                           ctor_param_names, ctor_param_origins,
                                           num_inputs, &ctx->ctor_builder);
  ctx->init_fn = audio_mir_build_bundle_fn(
      ctx, "init", init_type, init_params, init_param_names, NULL,
      sizeof(init_params) / sizeof(init_params[0]), &ctx->init_builder);
  ctx->kernel_fn = audio_mir_build_kernel_fn(ctx, ctor_params, ctor_param_names,
                                             ctor_param_origins, num_inputs);
  ctx->frame_fn = audio_mir_build_frame_fn(ctx);

  return ctx->ctor_fn && ctx->init_fn && ctx->kernel_fn && ctx->frame_fn;
}

static bool audio_mir_build_local_synth_functions(MirAudioSynthBuildCtx *ctx) {
  if (!ctx || !ctx->program || !ctx->program->arena) {
    return false;
  }

  MirArena *arena = ctx->arena ? ctx->arena : ctx->program->arena;
  size_t num_inputs = (size_t)ctx->num_inputs;
  size_t capture_count = ctx->capture_count;
  size_t kernel_param_count = capture_count + num_inputs;

  Type **kernel_params =
      kernel_param_count
          ? mir_arena_alloc(arena, sizeof(Type *) * kernel_param_count,
                            __alignof__(Type *))
          : NULL;
  const char **kernel_param_names =
      kernel_param_count
          ? mir_arena_alloc(arena, sizeof(char *) * kernel_param_count,
                            __alignof__(char *))
          : NULL;
  Ast **kernel_param_origins =
      kernel_param_count
          ? mir_arena_alloc(arena, sizeof(Ast *) * kernel_param_count,
                            __alignof__(Ast *))
          : NULL;

  if (kernel_param_count &&
      (!kernel_params || !kernel_param_names || !kernel_param_origins)) {
    return false;
  }

  for (size_t i = 0; i < capture_count; i++) {
    Ast *capture =
        ctx->capture_asts && ctx->capture_asts[i] ? ctx->capture_asts[i] : NULL;
    kernel_params[i] = capture && capture->type ? capture->type : &t_num;
    kernel_param_names[i] = audio_mir_capture_param_name(arena, capture, i);
    kernel_param_origins[i] = capture ? capture : ctx->app;
  }

  if (num_inputs) {
    if (!audio_mir_collect_ctor_params(ctx, kernel_params + capture_count,
                                       kernel_param_names + capture_count,
                                       kernel_param_origins + capture_count,
                                       num_inputs)) {
      return false;
    }
  }

  Type *ptr_char = audio_mir_ptr_to(arena, &t_char);
  Type *cons_params[] = {ptr_char, &t_int};
  const char *cons_param_names[] = {"parent_state", "state_offset"};
  Type *cons_type =
      audio_mir_fn_type(arena, cons_params,
                        sizeof(cons_params) / sizeof(cons_params[0]), ptr_char);
  Type *init_params[] = {ptr_char};
  const char *init_param_names[] = {"state"};
  Type *init_type =
      audio_mir_fn_type(arena, init_params,
                        sizeof(init_params) / sizeof(init_params[0]), &t_void);
  if (!cons_type || !init_type) {
    return false;
  }

  ctx->ctor_fn = audio_mir_build_bundle_fn(
      ctx, "cons", cons_type, cons_params, cons_param_names, NULL,
      sizeof(cons_params) / sizeof(cons_params[0]), &ctx->ctor_builder);
  ctx->init_fn = audio_mir_build_bundle_fn(
      ctx, "init", init_type, init_params, init_param_names, NULL,
      sizeof(init_params) / sizeof(init_params[0]), &ctx->init_builder);
  ctx->kernel_fn =
      audio_mir_build_kernel_fn(ctx, kernel_params, kernel_param_names,
                                kernel_param_origins, kernel_param_count);

  return ctx->ctor_fn && ctx->init_fn && ctx->kernel_fn;
}

static void audio_mir_compile_ctx_init(AudioCompileCtx *audio,
                                       MirAudioSynthBuildCtx *bundle) {
  if (!audio || !bundle) {
    return;
  }

  memset(audio, 0, sizeof(*audio));
  audio->bundle = bundle;
  audio->program = bundle->program;
  audio->arena = bundle->arena ? bundle->arena : bundle->program->arena;
  audio->app = bundle->app;
  audio->lambda = bundle->lambda;
  audio->parent_ctx = bundle->parent_ctx;
  audio->cons_builder = &bundle->ctor_builder;
  audio->init_builder = &bundle->init_builder;
  audio->kernel_builder = &bundle->kernel_builder;
  audio->frame_builder = &bundle->frame_builder;
  audio->cons_fn = bundle->ctor_fn;
  audio->init_fn = bundle->init_fn;
  audio->kernel_fn = bundle->kernel_fn;
  audio->frame_fn = bundle->frame_fn;
  if (bundle->ctor_fn) {
    audio->bundle_fns[audio->bundle_fns_len++] = bundle->ctor_fn;
  }
  if (bundle->init_fn) {
    audio->bundle_fns[audio->bundle_fns_len++] = bundle->init_fn;
  }
  if (bundle->kernel_fn) {
    audio->bundle_fns[audio->bundle_fns_len++] = bundle->kernel_fn;
  }
  if (bundle->frame_fn) {
    audio->bundle_fns[audio->bundle_fns_len++] = bundle->frame_fn;
  }
  audio->ptr_char_type = audio_mir_ptr_to(audio->arena, &t_char);
  audio->ptr_double_type = audio_mir_ptr_to(audio->arena, &t_num);
  audio->ptr_ptr_type = audio_mir_ptr_to(audio->arena, &t_ptr);
}

static AudioStateSlot *audio_mir_reserve_state_slot(AudioCompileCtx *audio,
                                                    Type *output_type,
                                                    size_t state_size,
                                                    size_t state_align,
                                                    const char *name) {
  if (!audio || !audio->arena) {
    return NULL;
  }

  if (state_size == 0) {
    state_size = (size_t)audio_mir_sizeof_type(output_type);
  }
  if (state_align == 0) {
    state_align = state_size >= 8 ? 8 : state_size;
  }

  AudioStateSlot *slot = mir_arena_alloc(audio->arena, sizeof(AudioStateSlot),
                                         __alignof__(AudioStateSlot));
  if (!slot) {
    return NULL;
  }

  size_t offset = audio_mir_align_up(audio->state_cursor, state_align);
  *slot = (AudioStateSlot){
      .offset = offset,
      .size = state_size,
      .align = state_align,
      .type = output_type ? output_type : &t_num,
      .name = name ? mir_arena_strdup(audio->arena, name) : NULL,
      .zero_bytes = false,
      .next = audio->state_slots,
  };
  audio->state_slots = slot;
  audio->state_cursor = offset + state_size;
  if (audio->bundle) {
    audio->bundle->state_bytes =
        (int)audio_mir_align_up(audio->state_cursor, 8);
  }
  return slot;
}

static AudioStateSlot *audio_mir_reserve_state_block(AudioCompileCtx *audio,
                                                     size_t state_size,
                                                     size_t state_align,
                                                     const char *name) {
  AudioStateSlot *slot =
      audio_mir_reserve_state_slot(audio, &t_char, state_size, state_align,
                                   name);
  if (slot) {
    slot->zero_bytes = true;
  }
  return slot;
}

static MirValueId audio_mir_state_slot_ptr(AudioCompileCtx *audio,
                                           MirBuilder *builder, Ast *origin,
                                           MirValueId state, size_t offset,
                                           Type *pointee) {
  if (!audio || !builder || state == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId offset_value = mir_const_int(builder, &t_int, origin, (int)offset);
  MirValueId byte_ptr = mir_ptr_offset(builder, audio->ptr_char_type, origin,
                                       state, offset_value);
  if (byte_ptr == MIR_NO_VALUE || !pointee || pointee == &t_char) {
    return byte_ptr;
  }

  Type *typed_ptr = audio_mir_ptr_to(audio->arena, pointee);
  if (types_equal(audio->ptr_char_type, typed_ptr)) {
    return byte_ptr;
  }
  return mir_primitive_cast(builder, audio->ptr_char_type, typed_ptr, origin,
                            byte_ptr);
}

static MirValueId audio_mir_zero_value(MirBuilder *builder, Type *type,
                                       Ast *origin) {
  if (!builder || !type) {
    return MIR_NO_VALUE;
  }

  switch (type->kind) {
  case T_BOOL:
    return mir_const_bool(builder, type, origin, false);
  case T_CHAR:
    return mir_const_char(builder, type, origin, 0);
  case T_INT:
    return mir_const_int(builder, type, origin, 0);
  case T_UINT64:
    return mir_const_uint64(builder, type, origin, 0);
  case T_NUM:
    return mir_const_double(builder, type, origin, 0.0);
  default:
    return mir_const_undef(builder, type, origin);
  }
}

static AudioValue audio_mir_value(Type *type, MirValueId value, int lanes) {
  return (AudioValue){
      .kind = AUDIO_VALUE_MIR,
      .type = type,
      .value = value,
      .lanes = lanes,
      .vec = NULL,
      .synth = NULL,
      .partial_synth = NULL,
      .partial = NULL,
  };
}

static AudioValue audio_mir_synth_value(Type *type,
                                        MirAudioSynthSymbol *synth) {
  if (!synth) {
    return AUDIO_VALUE_NULL;
  }
  return (AudioValue){
      .kind = AUDIO_VALUE_SYNTH,
      .type = type,
      .value = MIR_NO_VALUE,
      .lanes = 0,
      .vec = NULL,
      .synth = synth,
      .partial_synth = NULL,
      .partial = NULL,
  };
}

static AudioValue audio_mir_partial_synth_value(Type *type,
                                                AudioPartialSynth *partial) {
  if (!partial || !partial->synth) {
    return AUDIO_VALUE_NULL;
  }
  return (AudioValue){
      .kind = AUDIO_VALUE_PARTIAL_SYNTH,
      .type = type,
      .value = MIR_NO_VALUE,
      .lanes = 0,
      .vec = NULL,
      .synth = NULL,
      .partial_synth = partial,
      .partial = NULL,
  };
}

static bool audio_mir_value_is_valid(AudioValue value) {
  return value.kind == AUDIO_VALUE_SYNTH ||
         value.kind == AUDIO_VALUE_PARTIAL_SYNTH ||
         value.kind == AUDIO_VALUE_PARTIAL_BUILTIN ||
         value.value != MIR_NO_VALUE;
}

static int audio_mir_value_lane_count(AudioValue value) {
  return value.kind == AUDIO_VALUE_MIR && value.lanes > 0 ? value.lanes : 0;
}

static MirValueId audio_mir_value_lane(AudioValue value, int lane) {
  if (value.lanes > 1 && value.vec) {
    return value.vec[lane % value.lanes];
  }
  return value.value;
}

static Type *audio_mir_value_lane_type(AudioValue value, int lane) {
  if (audio_mir_is_tuple_type(value.type) && value.type->data.T_CONS.args &&
      value.type->data.T_CONS.num_args > 0) {
    return value.type->data.T_CONS
        .args[lane % value.type->data.T_CONS.num_args];
  }
  return value.type ? value.type : &t_num;
}

static bool audio_mir_is_primitive_type(Type *type) {
  return type && type->kind <= T_STRING;
}

static Type *audio_mir_array_element_type(Type *array_type) {
  if (!array_type || !is_array_type(array_type) ||
      !array_type->data.T_CONS.args || array_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *element_type = array_type->data.T_CONS.args[0];
  return element_type && element_type->kind != T_VAR ? element_type : &t_num;
}

static MirValueId audio_mir_cast_if_needed(MirBuilder *builder, Ast *origin,
                                           MirValueId value, Type *from_type,
                                           Type *to_type) {
  if (value == MIR_NO_VALUE || !from_type || !to_type ||
      types_equal(from_type, to_type)) {
    return value;
  }
  if (!audio_mir_is_primitive_type(from_type) ||
      !audio_mir_is_primitive_type(to_type)) {
    return value;
  }
  return mir_primitive_cast(builder, from_type, to_type, origin, value);
}

static void audio_mir_add_state_init(AudioCompileCtx *audio, size_t offset,
                                     Type *type, Ast *expr) {
  if (!audio || !audio->arena || !type || !expr) {
    return;
  }

  AudioStateInitStore *init =
      mir_arena_alloc(audio->arena, sizeof(AudioStateInitStore),
                      __alignof__(AudioStateInitStore));
  if (!init) {
    return;
  }

  *init = (AudioStateInitStore){
      .offset = offset,
      .type = type,
      .expr = expr,
      .next = audio->state_inits,
  };
  audio->state_inits = init;
}

static void audio_mir_add_init_call(AudioCompileCtx *audio,
                                    MirFunction *init_fn, size_t state_offset,
                                    Type *state_type, const char *name) {
  if (!audio || !audio->arena || !init_fn) {
    return;
  }

  AudioInitCall *call = mir_arena_alloc(audio->arena, sizeof(AudioInitCall),
                                        __alignof__(AudioInitCall));
  if (!call) {
    return;
  }

  *call = (AudioInitCall){
      .init_fn = init_fn,
      .state_offset = state_offset,
      .state_type = state_type ? state_type : audio->ptr_char_type,
      .name = name ? mir_arena_strdup(audio->arena, name) : NULL,
      .next = audio->init_calls,
  };
  audio->init_calls = call;
}

static MirValueId audio_mir_const_init_value(AudioCompileCtx *audio,
                                             Type *target_type, Ast *expr) {
  if (!audio || !audio->init_builder || !target_type || !expr) {
    return MIR_NO_VALUE;
  }

  MirBuilder *b = audio->init_builder;
  Type *from_type = target_type;
  MirValueId value = MIR_NO_VALUE;
  switch (expr->tag) {
  case AST_BOOL:
    from_type = &t_bool;
    value = mir_const_bool(b, from_type, expr, expr->data.AST_BOOL.value);
    break;
  case AST_CHAR:
    from_type = &t_char;
    value = mir_const_char(b, from_type, expr, expr->data.AST_CHAR.value);
    break;
  case AST_INT:
    from_type = &t_int;
    value = mir_const_int(b, from_type, expr, expr->data.AST_INT.value);
    break;
  case AST_UINT64:
    from_type = &t_uint64;
    value = mir_const_uint64(b, from_type, expr, expr->data.AST_UINT64.value);
    break;
  case AST_FLOAT:
    from_type = &t_num;
    value = mir_const_float(b, from_type, expr, expr->data.AST_FLOAT.value);
    break;
  case AST_DOUBLE:
    from_type = &t_num;
    value = mir_const_double(b, from_type, expr, expr->data.AST_DOUBLE.value);
    break;
  default:
    return audio_mir_zero_value(b, target_type, expr);
  }

  return audio_mir_cast_if_needed(b, expr, value, from_type, target_type);
}

static void audio_mir_emit_state_init_stores(AudioCompileCtx *audio,
                                             MirValueId state) {
  if (!audio || !audio->init_builder || state == MIR_NO_VALUE) {
    return;
  }

  for (AudioStateInitStore *init = audio->state_inits; init;
       init = init->next) {
    Ast *origin = init->expr ? init->expr : audio->app;
    MirValueId ptr = audio_mir_state_slot_ptr(
        audio, audio->init_builder, origin, state, init->offset, init->type);
    MirValueId value = audio_mir_const_init_value(audio, init->type, origin);
    if (ptr != MIR_NO_VALUE && value != MIR_NO_VALUE) {
      mir_ptr_store(audio->init_builder, origin, ptr, value);
    }
  }
}

static void audio_mir_emit_init_calls(AudioCompileCtx *audio,
                                      AudioInitCall *call,
                                      MirValueId parent_state) {
  if (!audio || !call || parent_state == MIR_NO_VALUE) {
    return;
  }

  audio_mir_emit_init_calls(audio, call->next, parent_state);

  MirValueId child_state =
      audio_mir_state_slot_ptr(audio, audio->init_builder, audio->app,
                               parent_state, call->state_offset, &t_char);
  if (child_state == MIR_NO_VALUE) {
    return;
  }
  if (call->state_type &&
      !types_equal(call->state_type, audio->ptr_char_type)) {
    child_state = mir_primitive_cast(audio->init_builder, audio->ptr_char_type,
                                     call->state_type, audio->app, child_state);
  }
  if (child_state == MIR_NO_VALUE) {
    return;
  }

  MirValueId init_ref =
      audio_mir_fn_ref(audio, audio->init_builder, call->init_fn->type,
                       audio->app, call->init_fn);
  mir_call_value(audio->init_builder, &t_void, audio->app, init_ref,
                 call->init_fn->type, (MirValueId[]){child_state}, 1);
}

static void audio_mir_emit_memzero(AudioCompileCtx *audio, Ast *origin,
                                   MirValueId ptr, size_t size) {
  if (!audio || !audio->init_builder || ptr == MIR_NO_VALUE || size == 0 ||
      size > (size_t)INT_MAX) {
    return;
  }

  Type *params[] = {audio->ptr_char_type, &t_int};
  Type *memzero_type =
      audio_mir_fn_type(audio->arena, params,
                        sizeof(params) / sizeof(params[0]), &t_void);
  MirValueId memzero_fn =
      audio_mir_extern_ref(audio->init_builder, "ylc_audio_memzero",
                           memzero_type, origin);
  MirValueId size_value =
      mir_const_int(audio->init_builder, &t_int, origin, (int)size);
  if (memzero_fn == MIR_NO_VALUE || size_value == MIR_NO_VALUE) {
    return;
  }

  MirValueId args[] = {ptr, size_value};
  mir_call_value(audio->init_builder, &t_void, origin, memzero_fn,
                 memzero_type, args, sizeof(args) / sizeof(args[0]));
}

static MirValueId audio_mir_array_view(AudioCompileCtx *audio, Ast *origin,
                                       Type *array_type, int size,
                                       MirValueId data_ptr) {
  if (!audio || !audio->kernel_builder || !array_type ||
      data_ptr == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueIdVec fields = {0};
  mir_value_id_vec_push(
      audio->arena, &fields,
      mir_const_int(audio->kernel_builder, &t_int, origin, size));
  mir_value_id_vec_push(
      audio->arena, &fields,
      mir_const_int(audio->kernel_builder, &t_int, origin, 0));
  mir_value_id_vec_push(audio->arena, &fields, data_ptr);
  return mir_tuple(audio->kernel_builder, array_type, origin, fields);
}

static AudioValue audio_mir_array_literal(AudioCompileCtx *audio, Ast *ast) {
  if (!audio || !ast || ast->tag != AST_ARRAY || !is_array_type(ast->type)) {
    return AUDIO_VALUE_NULL;
  }

  Type *element_type = audio_mir_array_element_type(ast->type);
  if (!element_type) {
    return AUDIO_VALUE_NULL;
  }

  int len = (int)ast->data.AST_LIST.len;
  int element_size = audio_mir_sizeof_type(element_type);
  size_t state_size = (size_t)(len > 0 ? len : 1) * (size_t)element_size;
  size_t state_align = element_size >= 8 ? 8 : (size_t)element_size;
  AudioStateSlot *slot = audio_mir_reserve_state_slot(
      audio, element_type, state_size, state_align, "array.literal");
  if (!slot) {
    return AUDIO_VALUE_NULL;
  }

  for (int i = 0; i < len; i++) {
    audio_mir_add_state_init(audio,
                             slot->offset + (size_t)i * (size_t)element_size,
                             element_type, ast->data.AST_LIST.items + i);
  }

  MirValueId data_ptr =
      audio_mir_state_slot_ptr(audio, audio->kernel_builder, ast,
                               audio->state_param, slot->offset, element_type);
  MirValueId array = audio_mir_array_view(audio, ast, ast->type, len, data_ptr);
  return audio_mir_value(ast->type, array, 1);
}

static AudioValue audio_mir_array_size_value(AudioCompileCtx *audio, Ast *app) {
  if (!audio || !app || app->data.AST_APPLICATION.len != 1) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue array = audio_mir_expr(audio, app->data.AST_APPLICATION.args);
  if (array.value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId size =
      mir_tuple_get(audio->kernel_builder, &t_int, app, array.value, 0);
  return audio_mir_value(&t_int, size, 1);
}

static AudioValue audio_mir_array_at_value(AudioCompileCtx *audio, Ast *app) {
  if (!audio || !app || app->data.AST_APPLICATION.len != 2) {
    return AUDIO_VALUE_NULL;
  }

  Ast *args = app->data.AST_APPLICATION.args;
  AudioValue array = audio_mir_expr(audio, args);
  AudioValue index = audio_mir_expr(audio, args + 1);
  Type *element_type =
      audio_mir_array_element_type(array.type ? array.type : args->type);
  if (array.value == MIR_NO_VALUE || index.value == MIR_NO_VALUE ||
      !element_type || audio_mir_value_lane_count(index) != 1) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId index_value =
      audio_mir_cast_if_needed(audio->kernel_builder, app, index.value,
                               audio_mir_value_lane_type(index, 0), &t_int);
  MirValueId array_offset =
      mir_tuple_get(audio->kernel_builder, &t_int, app, array.value, 1);
  MirValueId element_index =
      mir_iadd(audio->kernel_builder, &t_int, app, array_offset, index_value);
  Type *ptr_type = audio_mir_ptr_to(audio->arena, element_type);
  MirValueId data_ptr =
      mir_tuple_get(audio->kernel_builder, ptr_type, app, array.value, 2);
  MirValueId element_ptr = mir_ptr_offset(audio->kernel_builder, ptr_type, app,
                                          data_ptr, element_index);
  MirValueId element =
      mir_ptr_load(audio->kernel_builder, app->type ? app->type : element_type,
                   app, element_ptr);
  return audio_mir_value(app->type ? app->type : element_type, element, 1);
}

static AudioValue audio_mir_array_set_value(AudioCompileCtx *audio, Ast *app) {
  if (!audio || !app || app->data.AST_APPLICATION.len != 3) {
    return AUDIO_VALUE_NULL;
  }

  Ast *args = app->data.AST_APPLICATION.args;
  AudioValue array = audio_mir_expr(audio, args);
  AudioValue index = audio_mir_expr(audio, args + 1);
  AudioValue value = audio_mir_expr(audio, args + 2);
  Type *element_type =
      audio_mir_array_element_type(array.type ? array.type : args->type);
  if (array.value == MIR_NO_VALUE || index.value == MIR_NO_VALUE ||
      value.value == MIR_NO_VALUE || !element_type ||
      audio_mir_value_lane_count(index) != 1 ||
      audio_mir_value_lane_count(value) != 1) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId index_value =
      audio_mir_cast_if_needed(audio->kernel_builder, app, index.value,
                               audio_mir_value_lane_type(index, 0), &t_int);
  MirValueId array_offset =
      mir_tuple_get(audio->kernel_builder, &t_int, app, array.value, 1);
  MirValueId element_index =
      mir_iadd(audio->kernel_builder, &t_int, app, array_offset, index_value);
  MirValueId stored = audio_mir_cast_if_needed(
      audio->kernel_builder, args + 2, value.value,
      audio_mir_value_lane_type(value, 0), element_type);
  Type *ptr_type = audio_mir_ptr_to(audio->arena, element_type);
  MirValueId data_ptr =
      mir_tuple_get(audio->kernel_builder, ptr_type, app, array.value, 2);
  MirValueId element_ptr = mir_ptr_offset(audio->kernel_builder, ptr_type, app,
                                          data_ptr, element_index);
  if (mir_ptr_store(audio->kernel_builder, app, element_ptr, stored) ==
      MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  return array;
}

static MirValueId *audio_mir_alloc_lane_values(AudioCompileCtx *audio,
                                               int lanes) {
  if (!audio || !audio->arena || lanes <= 0) {
    return NULL;
  }
  return mir_arena_alloc(audio->arena, sizeof(MirValueId) * (size_t)lanes,
                         __alignof__(MirValueId));
}

static Type *audio_mir_tuple_type(AudioCompileCtx *audio, Type *preferred,
                                  Type *lane_type, int lanes) {
  if (audio_mir_is_tuple_type(preferred) &&
      preferred->data.T_CONS.num_args == lanes) {
    return preferred;
  }

  Type **items = mir_arena_alloc(audio->arena, sizeof(Type *) * (size_t)lanes,
                                 __alignof__(Type *));
  Type *type = mir_arena_alloc(audio->arena, sizeof(Type), __alignof__(Type));
  if (!items || !type) {
    return preferred ? preferred : &t_num;
  }

  for (int i = 0; i < lanes; i++) {
    items[i] = lane_type ? lane_type : &t_num;
  }
  memset(type, 0, sizeof(*type));
  type->kind = T_CONS;
  type->data.T_CONS.name = TYPE_NAME_TUPLE;
  type->data.T_CONS.args = items;
  type->data.T_CONS.num_args = lanes;
  return type;
}

static AudioValue audio_mir_multi_value_typed(AudioCompileCtx *audio,
                                              Ast *origin, Type *type,
                                              Type *lane_type,
                                              MirValueId *values, int lanes) {
  if (!values || lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes == 1) {
    return audio_mir_value(lane_type ? lane_type : &t_num, values[0], 1);
  }

  MirValueIdVec items = {0};
  for (int i = 0; i < lanes; i++) {
    mir_value_id_vec_push(audio->arena, &items, values[i]);
  }
  Type *tuple_type = audio_mir_tuple_type(audio, type, lane_type, lanes);
  MirValueId tuple =
      mir_tuple(audio->kernel_builder, tuple_type, origin, items);
  return (AudioValue){
      .kind = AUDIO_VALUE_MIR,
      .type = tuple_type,
      .value = tuple,
      .lanes = lanes,
      .vec = values,
      .synth = NULL,
      .partial_synth = NULL,
      .partial = NULL,
  };
}

static AudioValue audio_mir_multi_value(AudioCompileCtx *audio, Ast *origin,
                                        Type *type, MirValueId *values,
                                        int lanes) {
  return audio_mir_multi_value_typed(audio, origin, type, &t_num, values,
                                     lanes);
}

static AudioValue *audio_mir_lookup_local(AudioCompileCtx *audio,
                                          const char *name) {
  if (!audio || !name) {
    return NULL;
  }
  for (AudioLocalBinding *binding = audio->locals; binding;
       binding = binding->next) {
    if (binding->name && strcmp(binding->name, name) == 0) {
      return &binding->value;
    }
  }
  return NULL;
}

static bool audio_mir_bind_local_value(AudioCompileCtx *audio, Ast *binding,
                                       AudioValue value) {
  if (!audio || !binding || !audio_mir_value_is_valid(value)) {
    return false;
  }
  if (binding->tag == AST_PLACEHOLDER_ID) {
    return true;
  }
  if (binding->tag != AST_IDENTIFIER) {
    return false;
  }
  if (binding->data.AST_IDENTIFIER.length == 1 &&
      binding->data.AST_IDENTIFIER.value &&
      binding->data.AST_IDENTIFIER.value[0] == '_') {
    return true;
  }

  AudioLocalBinding *local = mir_arena_alloc(
      audio->arena, sizeof(AudioLocalBinding), __alignof__(AudioLocalBinding));
  if (!local) {
    return false;
  }
  local->name =
      mir_arena_strdup(audio->arena, binding->data.AST_IDENTIFIER.value);
  local->value = value;
  local->next = audio->locals;
  audio->locals = local;

  if (value.kind == AUDIO_VALUE_MIR && value.value != MIR_NO_VALUE &&
      audio->mir_ctx) {
    return mir_ctx_bind_value(audio->mir_ctx,
                              binding->data.AST_IDENTIFIER.value, value.value);
  }
  return true;
}

static bool audio_mir_is_ptr_type(Type *type) {
  return type && type->kind == T_CONS && type->data.T_CONS.name &&
         strcmp(type->data.T_CONS.name, TYPE_NAME_PTR) == 0;
}

static MirValueId audio_mir_value_as_node(MirBuilder *builder, Ast *origin,
                                          MirValueId value, Type *type) {
  if (audio_mir_is_ptr_type(type) || value == MIR_NO_VALUE) {
    return value;
  }

  if (!type || type->kind != T_NUM) {
    value = mir_primitive_cast(builder, type ? type : &t_num, &t_num, origin,
                               value);
  }

  Type *params[] = {&t_num};
  Type *const_sig_type = audio_mir_fn_type(
      builder->fn->arena, params, sizeof(params) / sizeof(params[0]), &t_ptr);
  MirValueId const_sig =
      audio_mir_extern_ref(builder, "const_sig", const_sig_type, origin);
  return mir_call_value(builder, &t_ptr, origin, const_sig, const_sig_type,
                        (MirValueId[]){value}, 1);
}

static MirValueId audio_mir_call_unary_node(MirBuilder *builder, Ast *origin,
                                            const char *name,
                                            MirValueId input) {
  Type *params[] = {&t_ptr};
  Type *type = audio_mir_fn_type(builder->fn->arena, params,
                                 sizeof(params) / sizeof(params[0]), &t_ptr);
  MirValueId node_fn = audio_mir_extern_ref(builder, name, type, origin);
  return mir_call_value(builder, &t_ptr, origin, node_fn, type,
                        (MirValueId[]){input}, 1);
}

static AudioValue audio_mir_wrap_mir_value(AudioCompileCtx *audio, Ast *origin,
                                           Type *type, MirValueId value) {
  if (!audio || !audio->kernel_builder || value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  type = type ? type : (origin && origin->type ? origin->type : &t_num);
  if (audio_mir_is_tuple_type(type) && type->data.T_CONS.args &&
      type->data.T_CONS.num_args > 0) {
    int lanes = type->data.T_CONS.num_args;
    MirValueId *values = audio_mir_alloc_lane_values(audio, lanes);
    if (values) {
      for (int i = 0; i < lanes; i++) {
        Type *field_type =
            type->data.T_CONS.args[i] ? type->data.T_CONS.args[i] : &t_num;
        values[i] = mir_tuple_get(audio->kernel_builder, field_type, origin,
                                  value, (size_t)i);
      }
      return (AudioValue){
          .kind = AUDIO_VALUE_MIR,
          .type = type,
          .value = value,
          .lanes = lanes,
          .vec = values,
          .synth = NULL,
          .partial_synth = NULL,
          .partial = NULL,
      };
    }
  }

  return audio_mir_value(type, value, 1);
}

static AudioValue audio_mir_mir_expr(AudioCompileCtx *audio, Ast *ast) {
  if (!audio || !audio->kernel_builder || !ast) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId value = mir_expr(audio->kernel_builder, ast, audio->mir_ctx);
  return audio_mir_wrap_mir_value(audio, ast, ast->type ? ast->type : &t_num,
                                  value);
}

static const char *audio_mir_application_name(Ast *app) {
  if (!app || app->tag != AST_APPLICATION) {
    return NULL;
  }
  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn && fn->tag == AST_IDENTIFIER) {
    return fn->data.AST_IDENTIFIER.value;
  }
  return NULL;
}

static const char *audio_mir_callable_name(Ast *fn) {
  if (!fn) {
    return NULL;
  }
  if (fn->tag == AST_IDENTIFIER) {
    return fn->data.AST_IDENTIFIER.value;
  }
  if (fn->tag == AST_RECORD_ACCESS && fn->data.AST_RECORD_ACCESS.member &&
      fn->data.AST_RECORD_ACCESS.member->tag == AST_IDENTIFIER) {
    return fn->data.AST_RECORD_ACCESS.member->data.AST_IDENTIFIER.value;
  }
  return NULL;
}

static const char *audio_mir_application_callable_name(Ast *app) {
  return app && app->tag == AST_APPLICATION
             ? audio_mir_callable_name(app->data.AST_APPLICATION.function)
             : NULL;
}

static bool audio_mir_application_has_unit_arg(Ast *app) {
  return app && app->tag == AST_APPLICATION &&
         app->data.AST_APPLICATION.len == 1 && app->data.AST_APPLICATION.args &&
         app->data.AST_APPLICATION.args->tag == AST_VOID;
}

static size_t audio_mir_application_value_arg_count(Ast *app) {
  return audio_mir_application_has_unit_arg(app)
             ? 0
             : (app ? (size_t)app->data.AST_APPLICATION.len : 0);
}

static MirAudioSynthSymbol *audio_mir_ast_audio_symbol(AudioCompileCtx *audio,
                                                       Ast *fn) {
  if (!audio || !audio->program || !audio->mir_ctx || !fn ||
      (fn->tag != AST_IDENTIFIER && fn->tag != AST_RECORD_ACCESS)) {
    return NULL;
  }

  MirSymbol *symbol =
      mir_resolve_ast_symbol(audio->kernel_builder, fn, audio->mir_ctx);
  if (!symbol && audio->parent_ctx && audio->parent_ctx != audio->mir_ctx) {
    symbol =
        mir_resolve_ast_symbol(audio->kernel_builder, fn, audio->parent_ctx);
  }
  if (!symbol || symbol->kind != MIR_SYMBOL_CUSTOM ||
      symbol->as.custom.handler != MirAudioSynthSymbolHandler) {
    return NULL;
  }

  return (MirAudioSynthSymbol *)symbol->as.custom.data;
}

static MirAudioSynthSymbol *
audio_mir_application_audio_symbol(AudioCompileCtx *audio, Ast *app) {
  if (!audio || !audio->program || !audio->mir_ctx || !app ||
      app->tag != AST_APPLICATION || !app->data.AST_APPLICATION.function) {
    return NULL;
  }

  Ast *fn = app->data.AST_APPLICATION.function;
  while (fn && fn->tag == AST_APPLICATION) {
    fn = fn->data.AST_APPLICATION.function;
  }
  if (fn->tag != AST_IDENTIFIER && fn->tag != AST_RECORD_ACCESS) {
    return NULL;
  }
  return audio_mir_ast_audio_symbol(audio, fn);
}

static bool audio_mir_application_is_partial(Ast *app) {
  return app && app->tag == AST_APPLICATION &&
         app->data.AST_APPLICATION.function &&
         app->data.AST_APPLICATION.function->type &&
         application_is_partial(app);
}

static Ast *audio_mir_application_root_function(Ast *app) {
  Ast *fn = app;
  while (fn && fn->tag == AST_APPLICATION) {
    fn = fn->data.AST_APPLICATION.function;
  }
  return fn;
}

static size_t audio_mir_application_flat_arg_count(Ast *app) {
  if (!app || app->tag != AST_APPLICATION) {
    return 0;
  }

  size_t count = app->data.AST_APPLICATION.len;
  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn && fn->tag == AST_APPLICATION) {
    count += audio_mir_application_flat_arg_count(fn);
  }
  return count;
}

static size_t audio_mir_application_collect_flat_args(Ast *app, Ast *args,
                                                      size_t offset) {
  if (!app || app->tag != AST_APPLICATION) {
    return offset;
  }

  Ast *fn = app->data.AST_APPLICATION.function;
  if (fn && fn->tag == AST_APPLICATION) {
    offset = audio_mir_application_collect_flat_args(fn, args, offset);
  }

  for (size_t i = 0; i < app->data.AST_APPLICATION.len; i++) {
    args[offset++] = app->data.AST_APPLICATION.args[i];
  }
  return offset;
}

static Ast *audio_mir_application_flatten_if_saturated(AudioCompileCtx *audio,
                                                       Ast *app) {
  if (!audio || !audio->arena || !app || app->tag != AST_APPLICATION ||
      !app->data.AST_APPLICATION.function ||
      app->data.AST_APPLICATION.function->tag != AST_APPLICATION) {
    return app;
  }

  Ast *root = audio_mir_application_root_function(app);
  if (!root || !root->type || root->type->kind != T_FN) {
    return app;
  }

  size_t arg_count = audio_mir_application_flat_arg_count(app);
  int expected = fn_type_args_len(root->type);
  if (expected <= 0 || arg_count > (size_t)expected) {
    return app;
  }

  Ast *flat = mir_arena_alloc(audio->arena, sizeof(Ast), __alignof__(Ast));
  Ast *args = arg_count ? mir_arena_alloc(audio->arena, sizeof(Ast) * arg_count,
                                          __alignof__(Ast))
                        : NULL;
  if (!flat || (arg_count && !args)) {
    return app;
  }

  *flat = *app;
  flat->data.AST_APPLICATION.function = root;
  flat->data.AST_APPLICATION.args = args;
  flat->data.AST_APPLICATION.len = arg_count;
  audio_mir_application_collect_flat_args(app, args, 0);
  return flat;
}

static Ast *audio_mir_application_flatten_any(AudioCompileCtx *audio,
                                              Ast *app) {
  if (!audio || !audio->arena || !app || app->tag != AST_APPLICATION ||
      !app->data.AST_APPLICATION.function ||
      app->data.AST_APPLICATION.function->tag != AST_APPLICATION) {
    return app;
  }

  Ast *root = audio_mir_application_root_function(app);
  size_t arg_count = audio_mir_application_flat_arg_count(app);
  Ast *flat = mir_arena_alloc(audio->arena, sizeof(Ast), __alignof__(Ast));
  Ast *args = arg_count ? mir_arena_alloc(audio->arena, sizeof(Ast) * arg_count,
                                          __alignof__(Ast))
                        : NULL;
  if (!flat || (arg_count && !args)) {
    return app;
  }

  *flat = *app;
  flat->data.AST_APPLICATION.function = root;
  flat->data.AST_APPLICATION.args = args;
  flat->data.AST_APPLICATION.len = arg_count;
  audio_mir_application_collect_flat_args(app, args, 0);
  return flat;
}

static const char *audio_mir_osc_node_ctor_name(const char *name) {
  if (!name) {
    return NULL;
  }
  if (strcmp(name, "sin_osc") == 0) {
    return "sin_node";
  }
  if (strcmp(name, "sq_osc") == 0) {
    return "sq_node";
  }
  if (strcmp(name, "saw_osc") == 0) {
    return "saw_node";
  }

  if (strcmp(name, "pm_osc") == 0) {
    return "pm_node";
  }
  return NULL;
}

static MirValueId audio_mir_num_lane(AudioCompileCtx *audio, Ast *origin,
                                     AudioValue value, int lane) {
  MirValueId lane_value = audio_mir_value_lane(value, lane);
  Type *lane_type = audio_mir_value_lane_type(value, lane);
  if (!lane_type || lane_type->kind != T_NUM) {
    lane_value = mir_primitive_cast(audio->kernel_builder,
                                    lane_type ? lane_type : &t_num, &t_num,
                                    origin, lane_value);
  }
  return lane_value;
}

static int audio_mir_max_lanes(const AudioValue *values, size_t len) {
  if (!values || len == 0) {
    return 0;
  }

  int lanes = 1;
  for (size_t i = 0; i < len; i++) {
    int value_lanes = audio_mir_value_lane_count(values[i]);
    if (value_lanes <= 0) {
      return 0;
    }
    if (value_lanes > lanes) {
      lanes = value_lanes;
    }
  }
  return lanes;
}

static bool audio_mir_normalize_num_kernel_args_masked(
    AudioCompileCtx *audio, Ast *origin, const AudioValue *values,
    AudioMirKernelArgLanes *args, size_t argc, uint64_t lane_expand_mask,
    int forced_lanes, int *out_lanes) {
  if (!audio || !origin || !values || !args || argc == 0 || !out_lanes) {
    return false;
  }

  int lanes = forced_lanes > 0 ? forced_lanes : 1;
  for (size_t i = 0; i < argc; i++) {
    int value_lanes = audio_mir_value_lane_count(values[i]);
    if (value_lanes <= 0) {
      return false;
    }
    if (!forced_lanes && (lane_expand_mask & AUDIO_ARG_MASK(i)) &&
        value_lanes > lanes) {
      lanes = value_lanes;
    }
  }
  if (lanes <= 0) {
    return false;
  }

  for (size_t i = 0; i < argc; i++) {
    MirValueId *lane_values = audio_mir_alloc_lane_values(audio, lanes);
    if (!lane_values) {
      return false;
    }

    for (int lane = 0; lane < lanes; lane++) {
      int source_lane = (lane_expand_mask & AUDIO_ARG_MASK(i)) ? lane : 0;
      lane_values[lane] =
          audio_mir_num_lane(audio, origin, values[i], source_lane);
      if (lane_values[lane] == MIR_NO_VALUE) {
        return false;
      }
    }

    args[i] = (AudioMirKernelArgLanes){
        .type = &t_num,
        .values = lane_values,
    };
  }

  *out_lanes = lanes;
  return true;
}

static bool audio_mir_normalize_num_kernel_args(AudioCompileCtx *audio,
                                                Ast *origin,
                                                const AudioValue *values,
                                                AudioMirKernelArgLanes *args,
                                                size_t argc, int *out_lanes) {
  return audio_mir_normalize_num_kernel_args_masked(
      audio, origin, values, args, argc, AUDIO_ARG_MASK_ALL(argc), 0,
      out_lanes);
}

static bool audio_mir_value_is_const_zero(AudioCompileCtx *audio,
                                          MirValueId value) {
  if (!audio || !audio->kernel_builder || !audio->kernel_builder->fn ||
      value == MIR_NO_VALUE) {
    return false;
  }

  for (int depth = 0; depth < 4; depth++) {
    MirInstr *instr =
        mir_function_find_def_instr(audio->kernel_builder->fn, value);
    if (!instr) {
      return false;
    }

    if (instr->kind == MIR_CONST) {
      switch (instr->data.const_value.kind) {
      case MIR_CONST_KIND_INT:
        return instr->data.const_value.as.int_value == 0;
      case MIR_CONST_KIND_UINT64:
        return instr->data.const_value.as.uint64_value == 0;
      case MIR_CONST_KIND_FLOAT:
        return instr->data.const_value.as.float_value == 0.0f;
      case MIR_CONST_KIND_DOUBLE:
        return instr->data.const_value.as.double_value == 0.0;
      default:
        return false;
      }
    }

    if (instr->kind == MIR_OP && instr->data.op.kind == MIR_OP_KIND_CAST &&
        instr->data.op.argc == 1) {
      value = instr->data.op.operands[0];
      continue;
    }

    return false;
  }

  return false;
}

static bool audio_mir_value_const_int(AudioCompileCtx *audio, MirValueId value,
                                      int *out_value) {
  if (!audio || !audio->kernel_builder || !audio->kernel_builder->fn ||
      value == MIR_NO_VALUE || !out_value) {
    return false;
  }

  for (int depth = 0; depth < 4; depth++) {
    MirInstr *instr =
        mir_function_find_def_instr(audio->kernel_builder->fn, value);
    if (!instr) {
      return false;
    }

    if (instr->kind == MIR_CONST) {
      switch (instr->data.const_value.kind) {
      case MIR_CONST_KIND_INT:
        *out_value = instr->data.const_value.as.int_value;
        return true;
      case MIR_CONST_KIND_UINT64:
        if (instr->data.const_value.as.uint64_value > (uint64_t)INT_MAX) {
          return false;
        }
        *out_value = (int)instr->data.const_value.as.uint64_value;
        return true;
      default:
        return false;
      }
    }

    if (instr->kind == MIR_OP && instr->data.op.kind == MIR_OP_KIND_CAST &&
        instr->data.op.argc == 1) {
      value = instr->data.op.operands[0];
      continue;
    }

    return false;
  }

  return false;
}

static bool audio_mir_value_const_double(AudioCompileCtx *audio,
                                         MirValueId value, double *out_value) {
  if (!audio || !audio->kernel_builder || !audio->kernel_builder->fn ||
      value == MIR_NO_VALUE || !out_value) {
    return false;
  }

  for (int depth = 0; depth < 4; depth++) {
    MirInstr *instr =
        mir_function_find_def_instr(audio->kernel_builder->fn, value);
    if (!instr) {
      return false;
    }

    if (instr->kind == MIR_CONST) {
      switch (instr->data.const_value.kind) {
      case MIR_CONST_KIND_INT:
        *out_value = (double)instr->data.const_value.as.int_value;
        return true;
      case MIR_CONST_KIND_UINT64:
        *out_value = (double)instr->data.const_value.as.uint64_value;
        return true;
      case MIR_CONST_KIND_FLOAT:
        *out_value = (double)instr->data.const_value.as.float_value;
        return true;
      case MIR_CONST_KIND_DOUBLE:
        *out_value = instr->data.const_value.as.double_value;
        return true;
      default:
        return false;
      }
    }

    if (instr->kind == MIR_OP && instr->data.op.kind == MIR_OP_KIND_CAST &&
        instr->data.op.argc == 1) {
      value = instr->data.op.operands[0];
      continue;
    }

    return false;
  }

  return false;
}

static AudioValue audio_mir_binary_signal_op(AudioCompileCtx *audio, Ast *app,
                                             MirPrimitiveOp op,
                                             Type *result_type) {
  if (!audio || !app || app->tag != AST_APPLICATION ||
      app->data.AST_APPLICATION.len != 2 || !result_type) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue values[] = {
      audio_mir_expr(audio, app->data.AST_APPLICATION.args),
      audio_mir_expr(audio, app->data.AST_APPLICATION.args + 1),
  };
  AudioMirKernelArgLanes args[2];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, app, values, args, 2,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *results = audio_mir_alloc_lane_values(audio, lanes);
  if (!results) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId operands[] = {args[0].values[lane], args[1].values[lane]};
    results[lane] =
        mir_primitive_instr(audio->kernel_builder, op, result_type, app,
                            operands, sizeof(operands) / sizeof(operands[0]));
    if (results[lane] == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
  }

  return audio_mir_multi_value_typed(audio, app, app->type, result_type,
                                     results, lanes);
}

static AudioValue sum_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FADD, &t_num);
}

static AudioValue sub_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FSUB, &t_num);
}

static AudioValue mul_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FMUL, &t_num);
}

static AudioValue div_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FDIV, &t_num);
}

static AudioValue mod_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FMOD, &t_num);
}

static AudioValue gte_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FGTE, &t_bool);
}

static AudioValue gt_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FGT, &t_bool);
}

static AudioValue lte_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FLTE, &t_bool);
}

static AudioValue lt_signals(AudioCompileCtx *audio, Ast *app) {
  return audio_mir_binary_signal_op(audio, app, MIR_OP_FLT, &t_bool);
}

static Type *audio_mir_fn_arg_type(Type *fn_type, size_t index) {
  Type *cursor = fn_type;
  for (size_t i = 0; cursor && cursor->kind == T_FN;
       i++, cursor = cursor->data.T_FN.to) {
    if (i == index) {
      return cursor->data.T_FN.from;
    }
  }
  return NULL;
}

static Type *audio_mir_application_result_type(Ast *app, Type *fn_type) {
  if (app && app->type) {
    return app->type;
  }

  Type *cursor = fn_type;
  size_t argc = app ? app->data.AST_APPLICATION.len : 0;
  for (size_t i = 0; i < argc && cursor && cursor->kind == T_FN; i++) {
    cursor = cursor->data.T_FN.to;
  }
  return cursor ? cursor : &t_void;
}

static MirValueId audio_mir_lane_as_formal(AudioCompileCtx *audio, Ast *origin,
                                           AudioValue value, int lane,
                                           Type *formal) {
  MirValueId lane_value = audio_mir_value_lane(value, lane);
  Type *lane_type = audio_mir_value_lane_type(value, lane);
  if (lane_value == MIR_NO_VALUE || !formal || !lane_type ||
      types_equal(formal, lane_type) || !audio_mir_is_primitive_type(formal) ||
      !audio_mir_is_primitive_type(lane_type)) {
    return lane_value;
  }

  return mir_primitive_cast(audio->kernel_builder, lane_type, formal, origin,
                            lane_value);
}

static AudioValue audio_mir_call_application(AudioCompileCtx *audio, Ast *app) {
  if (!audio || !app || app->tag != AST_APPLICATION ||
      !app->data.AST_APPLICATION.function ||
      !app->data.AST_APPLICATION.function->type) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *callee_type = app->data.AST_APPLICATION.function->type;
  if (!callee_type || callee_type->kind != T_FN) {
    return audio_mir_mir_expr(audio, app);
  }

  MirValueId callee =
      mir_expr(b, app->data.AST_APPLICATION.function, audio->mir_ctx);
  if (callee == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  size_t argc = audio_mir_application_value_arg_count(app);
  AudioValue *args =
      argc ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                             __alignof__(AudioValue))
           : NULL;
  if (argc && !args) {
    return AUDIO_VALUE_NULL;
  }

  int lanes = 1;
  for (size_t i = 0; i < argc; i++) {
    args[i] = audio_mir_expr(audio, app->data.AST_APPLICATION.args + i);
    int arg_lanes = audio_mir_value_lane_count(args[i]);
    if (arg_lanes <= 0) {
      return AUDIO_VALUE_NULL;
    }
    if (arg_lanes > lanes) {
      lanes = arg_lanes;
    }
  }

  Type *result_type = audio_mir_application_result_type(app, callee_type);
  MirValueId *results =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !results) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId *call_args =
        argc ? mir_arena_alloc(audio->arena, sizeof(MirValueId) * argc,
                               __alignof__(MirValueId))
             : NULL;
    if (argc && !call_args) {
      return AUDIO_VALUE_NULL;
    }

    for (size_t i = 0; i < argc; i++) {
      call_args[i] = audio_mir_lane_as_formal(
          audio, app->data.AST_APPLICATION.args + i, args[i], lane,
          audio_mir_fn_arg_type(callee_type, i));
      if (call_args[i] == MIR_NO_VALUE) {
        return AUDIO_VALUE_NULL;
      }
    }

    MirValueId result = mir_call_value(b, result_type, app, callee, callee_type,
                                       call_args, argc);
    if (result == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(result_type, result, 1);
    }
    results[lane] = result;
  }

  return audio_mir_multi_value(audio, app, app->type, results, lanes);
}

static AudioValue audio_mir_emit_kernel_call_lanes(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const char *state_name, size_t state_size, size_t state_align, int lanes,
    const AudioMirKernelArgLanes *args, size_t argc) {
  if (!audio || !origin || !kernel_symbol || !state_name || lanes <= 0 ||
      (argc && !args)) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  size_t params_len = argc + 2;
  Type **params = mir_arena_alloc(audio->arena, sizeof(Type *) * params_len,
                                  __alignof__(Type *));
  if (!params) {
    return AUDIO_VALUE_NULL;
  }
  params[0] = audio->ptr_double_type;
  params[1] = &t_num;
  for (size_t i = 0; i < argc; i++) {
    params[i + 2] = args[i].type ? args[i].type : &t_num;
  }

  Type *kernel_type =
      audio_mir_fn_type(audio->arena, params, params_len, &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);

  for (int lane = 0; lane < lanes; lane++) {
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "%s.%d", state_name, lane)
                  : state_name;
    AudioStateSlot *slot = audio_mir_reserve_state_slot(
        audio, &t_num, state_size, state_align, slot_name);
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_num);

    MirValueId *call_args = mir_arena_alloc(
        audio->arena, sizeof(MirValueId) * params_len, __alignof__(MirValueId));
    if (!call_args) {
      return AUDIO_VALUE_NULL;
    }
    call_args[0] = state_ptr;
    call_args[1] = audio->spf_param;
    for (size_t i = 0; i < argc; i++) {
      call_args[i + 2] = args[i].values[lane];
    }

    MirValueId sample = mir_call_value(b, &t_num, origin, kernel_fn,
                                       kernel_type, call_args, params_len);
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static MirValueId audio_mir_state_base_arg(AudioCompileCtx *audio,
                                           Ast *origin) {
  if (!audio || !audio->kernel_builder || !audio->ptr_double_type) {
    return MIR_NO_VALUE;
  }
  if (audio->state_param == MIR_NO_VALUE) {
    return mir_const_undef(audio->kernel_builder, audio->ptr_double_type,
                           origin);
  }
  return mir_primitive_cast(audio->kernel_builder, audio->ptr_char_type,
                            audio->ptr_double_type, origin, audio->state_param);
}

static AudioValue audio_mir_emit_stateless_kernel_call_lanes(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol, int lanes,
    const AudioMirKernelArgLanes *args, size_t argc) {
  if (!audio || !origin || !kernel_symbol || lanes <= 0 || (argc && !args)) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  size_t params_len = argc + 2;
  Type **params = mir_arena_alloc(audio->arena, sizeof(Type *) * params_len,
                                  __alignof__(Type *));
  if (!params) {
    return AUDIO_VALUE_NULL;
  }
  params[0] = audio->ptr_double_type;
  params[1] = &t_num;
  for (size_t i = 0; i < argc; i++) {
    params[i + 2] = args[i].type ? args[i].type : &t_num;
  }

  Type *kernel_type =
      audio_mir_fn_type(audio->arena, params, params_len, &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  MirValueId state_arg = audio_mir_state_base_arg(audio, origin);

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId *call_args = mir_arena_alloc(
        audio->arena, sizeof(MirValueId) * params_len, __alignof__(MirValueId));
    if (!call_args) {
      return AUDIO_VALUE_NULL;
    }
    call_args[0] = state_arg;
    call_args[1] = audio->spf_param;
    for (size_t i = 0; i < argc; i++) {
      call_args[i + 2] = args[i].values[lane];
    }

    MirValueId sample = mir_call_value(b, &t_num, origin, kernel_fn,
                                       kernel_type, call_args, params_len);
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_num_state_kernel_masked(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const char *state_name, size_t state_size, size_t state_align,
    const AudioValue *values, size_t argc, uint64_t lane_expand_mask);

static AudioValue audio_mir_emit_num_stateless_kernel_masked(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const AudioValue *values, size_t argc, uint64_t lane_expand_mask);

static AudioValue audio_mir_emit_num_state_kernel(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const char *state_name, size_t state_size, size_t state_align,
    const AudioValue *values, size_t argc) {
  return audio_mir_emit_num_state_kernel_masked(
      audio, origin, kernel_symbol, state_name, state_size, state_align, values,
      argc, AUDIO_ARG_MASK_ALL(argc));
}

static AudioValue audio_mir_emit_num_state_kernel_masked(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const char *state_name, size_t state_size, size_t state_align,
    const AudioValue *values, size_t argc, uint64_t lane_expand_mask) {
  AudioMirKernelArgLanes *args =
      mir_arena_alloc(audio->arena, sizeof(AudioMirKernelArgLanes) * argc,
                      __alignof__(AudioMirKernelArgLanes));
  int lanes = 0;
  if (!args ||
      !audio_mir_normalize_num_kernel_args_masked(
          audio, origin, values, args, argc, lane_expand_mask, 0, &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(audio, origin, kernel_symbol,
                                          state_name, state_size, state_align,
                                          lanes, args, argc);
}

static AudioValue audio_mir_emit_num_stateless_kernel(AudioCompileCtx *audio,
                                                      Ast *origin,
                                                      const char *kernel_symbol,
                                                      const AudioValue *values,
                                                      size_t argc) {
  return audio_mir_emit_num_stateless_kernel_masked(
      audio, origin, kernel_symbol, values, argc, AUDIO_ARG_MASK_ALL(argc));
}

static AudioValue audio_mir_emit_num_stateless_kernel_masked(
    AudioCompileCtx *audio, Ast *origin, const char *kernel_symbol,
    const AudioValue *values, size_t argc, uint64_t lane_expand_mask) {
  AudioMirKernelArgLanes *args =
      mir_arena_alloc(audio->arena, sizeof(AudioMirKernelArgLanes) * argc,
                      __alignof__(AudioMirKernelArgLanes));
  int lanes = 0;
  if (!args ||
      !audio_mir_normalize_num_kernel_args_masked(
          audio, origin, values, args, argc, lane_expand_mask, 0, &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_stateless_kernel_call_lanes(
      audio, origin, kernel_symbol, lanes, args, argc);
}

static AudioValue multichannel_operator(AudioCompileCtx *audio, Ast *app) {
  Ast *list = app ? app->data.AST_APPLICATION.args : NULL;

  if (!audio || !list || (list->tag != AST_LIST && list->tag != AST_ARRAY)) {
    return AUDIO_VALUE_NULL;
  }

  int len = list->data.AST_LIST.len;
  if (len <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *elements = audio_mir_alloc_lane_values(audio, len);
  if (!elements) {
    return AUDIO_VALUE_NULL;
  }

  for (int i = 0; i < len; i++) {
    AudioValue expr = audio_mir_expr(audio, list->data.AST_LIST.items + i);
    if (audio_mir_value_lane_count(expr) != 1 || expr.value == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    elements[i] = expr.value;
  }

  return audio_mir_multi_value(audio, app, app->type, elements, len);
}

static AudioValue audio_builtin_emit_num_state(const AudioBuiltin *builtin,
                                               AudioCompileCtx *audio,
                                               Ast *origin, AudioValue *args,
                                               size_t argc);
static AudioValue audio_builtin_emit_num_stateless(const AudioBuiltin *builtin,
                                                   AudioCompileCtx *audio,
                                                   Ast *origin,
                                                   AudioValue *args,
                                                   size_t argc);
static AudioValue audio_builtin_emit_trig(const AudioBuiltin *builtin,
                                          AudioCompileCtx *audio, Ast *origin,
                                          AudioValue *args, size_t argc);
static AudioValue audio_builtin_emit_kill_on_end(const AudioBuiltin *builtin,
                                                 AudioCompileCtx *audio,
                                                 Ast *origin, AudioValue *args,
                                                 size_t argc);
static AudioValue audio_builtin_emit_tabread(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc);
static AudioValue audio_builtin_emit_tabread_samp(const AudioBuiltin *builtin,
                                                  AudioCompileCtx *audio,
                                                  Ast *origin, AudioValue *args,
                                                  size_t argc);
static AudioValue audio_builtin_emit_bufplay(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc);
static AudioValue audio_builtin_emit_mbufplay(const AudioBuiltin *builtin,
                                              AudioCompileCtx *audio,
                                              Ast *origin, AudioValue *args,
                                              size_t argc);
static AudioValue audio_builtin_emit_delay_line(const AudioBuiltin *builtin,
                                                AudioCompileCtx *audio,
                                                Ast *origin, AudioValue *args,
                                                size_t argc);
static AudioValue audio_builtin_emit_array_trigger(const AudioBuiltin *builtin,
                                                   AudioCompileCtx *audio,
                                                   Ast *origin,
                                                   AudioValue *args,
                                                   size_t argc);
static AudioValue audio_builtin_emit_grains(const AudioBuiltin *builtin,
                                            AudioCompileCtx *audio,
                                            Ast *origin, AudioValue *args,
                                            size_t argc);
static AudioValue audio_builtin_emit_grains_env(const AudioBuiltin *builtin,
                                                AudioCompileCtx *audio,
                                                Ast *origin, AudioValue *args,
                                                size_t argc);
static AudioValue audio_builtin_emit_array_of_buf(const AudioBuiltin *builtin,
                                                  AudioCompileCtx *audio,
                                                  Ast *origin, AudioValue *args,
                                                  size_t argc);
static AudioValue audio_builtin_emit_bufsize(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc);

static const size_t audio_builtin_pm_osc_arg_order[] = {1, 0, 2};

static const AudioBuiltin audio_builtins[] = {
    {.name = "sin_osc",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_sin_osc_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(SinOscState),
     .state_align = __alignof__(SinOscState),
     .state_name = "sin_osc.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "sq_osc",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_sq_osc_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(SqOscState),
     .state_align = __alignof__(SqOscState),
     .state_name = "sq_osc.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "saw_osc",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_saw_osc_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(SawOscState),
     .state_align = __alignof__(SawOscState),
     .state_name = "saw_osc.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "phasor",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_phasor_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(PhasorState),
     .state_align = __alignof__(PhasorState),
     .state_name = "phasor.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "phasor_sinc",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_phasor_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(PhasorState),
     .state_align = __alignof__(PhasorState),
     .state_name = "phasor.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 1},

    {.name = "trig",
     .source_argc = 1,
     .kernel_symbol = NULL,
     .emit = audio_builtin_emit_trig,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "changed",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_changed_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(ChangedState),
     .state_align = __alignof__(ChangedState),
     .state_name = "changed.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "kill_on_end",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_kill_on_end_kernel",
     .emit = audio_builtin_emit_kill_on_end,
     .state_size = sizeof(KillOnEndState),
     .state_align = __alignof__(KillOnEndState),
     .state_name = "kill_on_end.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "decay",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_decay_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(DecayState),
     .state_align = __alignof__(DecayState),
     .state_name = "decay.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "rect",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_rect_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(RectState),
     .state_align = __alignof__(RectState),
     .state_name = "rect.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "lag",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_lag_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(LagState),
     .state_align = __alignof__(LagState),
     .state_name = "lag.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "comb",
     .source_argc = 4,
     .kernel_symbol = "ylc_audio_comb_kernel",
     .emit = audio_builtin_emit_delay_line,
     .state_size = 0,
     .state_align = 8,
     .state_name = "comb.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "dl_allpass",
     .source_argc = 4,
     .kernel_symbol = "ylc_audio_dl_allpass_kernel",
     .emit = audio_builtin_emit_delay_line,
     .state_size = 0,
     .state_align = 8,
     .state_name = "dl_allpass.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "allpass",
     .source_argc = 4,
     .kernel_symbol = "ylc_audio_dl_allpass_kernel",
     .emit = audio_builtin_emit_delay_line,
     .state_size = 0,
     .state_align = 8,
     .state_name = "dl_allpass.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "arr_choose",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_arr_choose_kernel",
     .emit = audio_builtin_emit_array_trigger,
     .state_size = sizeof(ArrayChooseState),
     .state_align = __alignof__(ArrayChooseState),
     .state_name = "arr_choose.state",
     .lane_expand_mask = AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "arr_seq",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_arr_seq_kernel",
     .emit = audio_builtin_emit_array_trigger,
     .state_size = sizeof(ArraySeqState),
     .state_align = __alignof__(ArraySeqState),
     .state_name = "arr_seq.state",
     .lane_expand_mask = AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "tabread",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_tabread_kernel",
     .emit = audio_builtin_emit_tabread,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask = AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "tabread_samp",
     .source_argc = 2,
     .kernel_symbol = "ylc_audio_tabread_samp_kernel",
     .emit = audio_builtin_emit_tabread_samp,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask = AUDIO_ARG_MASK(1),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "bufplay",
     .source_argc = 4,
     .kernel_symbol = "ylc_audio_bufplay_kernel",
     .emit = audio_builtin_emit_bufplay,
     .state_size = sizeof(BufplayState),
     .state_align = __alignof__(BufplayState),
     .state_name = "bufplay.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "mbufplay",
     .source_argc = 5,
     .kernel_symbol = "ylc_audio_mbufplay_kernel",
     .emit = audio_builtin_emit_mbufplay,
     .state_size = sizeof(BufplayState),
     .state_align = __alignof__(BufplayState),
     .state_name = "mbufplay.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3) | AUDIO_ARG_MASK(4),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "grains",
     .source_argc = 6,
     .kernel_symbol = "ylc_audio_grains_kernel",
     .emit = audio_builtin_emit_grains,
     .state_size = 0,
     .state_align = 8,
     .state_name = "grains.state",
     .lane_expand_mask = AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3) |
                         AUDIO_ARG_MASK(4) | AUDIO_ARG_MASK(5),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "grains_env",
     .source_argc = 7,
     .kernel_symbol = "ylc_audio_grains_env_kernel",
     .emit = audio_builtin_emit_grains_env,
     .state_size = 0,
     .state_align = 8,
     .state_name = "grains_env.state",
     .lane_expand_mask = AUDIO_ARG_MASK(3) | AUDIO_ARG_MASK(4) |
                         AUDIO_ARG_MASK(5) | AUDIO_ARG_MASK(6),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "pm_osc",
     .source_argc = 3,
     .kernel_symbol = "ylc_audio_pm_osc_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(PmOscState),
     .state_align = __alignof__(PmOscState),
     .state_name = "pm_osc.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2),
     .arg_order = audio_builtin_pm_osc_arg_order,
     .kernel_argc = 3},

    {.name = "scale",
     .source_argc = 3,
     .kernel_symbol = "ylc_audio_scale_kernel",
     .emit = audio_builtin_emit_num_stateless,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "scale_bp",
     .source_argc = 3,
     .kernel_symbol = "ylc_audio_scale_bp_kernel",
     .emit = audio_builtin_emit_num_stateless,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "lfnoise",
     .source_argc = 3,
     .kernel_symbol = "ylc_audio_lfnoise_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(LfNoiseState),
     .state_align = __alignof__(LfNoiseState),
     .state_name = "lfnoise.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "lfnoise0",
     .source_argc = 3,
     .kernel_symbol = "ylc_audio_lfnoise0_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(LfNoise0State),
     .state_align = __alignof__(LfNoise0State),
     .state_name = "lfnoise0.state",
     .lane_expand_mask =
         AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) | AUDIO_ARG_MASK(2),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "adsr",
     .source_argc = 5,
     .kernel_symbol = "ylc_audio_adsr_kernel",
     .emit = audio_builtin_emit_num_state,
     .state_size = sizeof(AdsrState),
     .state_align = __alignof__(AdsrState),
     .state_name = "adsr.state",
     .lane_expand_mask = AUDIO_ARG_MASK(0) | AUDIO_ARG_MASK(1) |
                         AUDIO_ARG_MASK(2) | AUDIO_ARG_MASK(3) |
                         AUDIO_ARG_MASK(4),
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "array_of_buf",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_array_of_buf",
     .emit = audio_builtin_emit_array_of_buf,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask = 0,
     .arg_order = NULL,
     .kernel_argc = 0},

    {.name = "bufsize",
     .source_argc = 1,
     .kernel_symbol = "ylc_audio_bufsize",
     .emit = audio_builtin_emit_bufsize,
     .state_size = 0,
     .state_align = 0,
     .state_name = NULL,
     .lane_expand_mask = 0,
     .arg_order = NULL,
     .kernel_argc = 0},
};

static const AudioBuiltin *audio_mir_find_builtin(const char *name) {
  if (!name) {
    return NULL;
  }
  size_t count = sizeof(audio_builtins) / sizeof(audio_builtins[0]);
  for (size_t i = 0; i < count; i++) {
    if (audio_builtins[i].name && strcmp(audio_builtins[i].name, name) == 0) {
      return &audio_builtins[i];
    }
  }
  return NULL;
}

static int audio_mir_fn_type_arity(Type *type) {
  if (!type || type->kind != T_FN) {
    return 0;
  }
  if (type->data.T_FN.from && type->data.T_FN.from->kind == T_VOID) {
    return 0;
  }

  int arity = 0;
  for (Type *cursor = type;
       cursor && cursor->kind == T_FN && !is_closure(cursor->data.T_FN.to);
       cursor = cursor->data.T_FN.to) {
    arity++;
  }
  return arity;
}

static int audio_mir_builtin_arity(AudioCompileCtx *audio, const char *name) {
  const AudioBuiltin *builtin = audio_mir_find_builtin(name);
  if (!audio || !builtin) {
    return 0;
  }

  MirProgram *program =
      audio->kernel_builder ? audio->kernel_builder->program : audio->program;
  MirSymbol *symbol = mir_ctx_lookup_symbol(program, audio->mir_ctx, name);
  if (symbol) {
    return audio_mir_fn_type_arity(symbol->type);
  }
  return builtin->source_argc > (size_t)INT_MAX ? 0 : (int)builtin->source_argc;
}

static AudioValue audio_mir_emit_sin_osc_value(AudioCompileCtx *audio,
                                               Ast *origin, AudioValue freq) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_sin_osc_kernel", "sin_osc.state",
      sizeof(SinOscState), __alignof__(SinOscState), lanes, args, 1);
}

static AudioValue audio_mir_emit_sq_osc_value(AudioCompileCtx *audio,
                                              Ast *origin, AudioValue freq) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_sq_osc_kernel", "sq_osc.state",
      sizeof(SqOscState), __alignof__(SqOscState), lanes, args, 1);
}

static AudioValue audio_mir_emit_saw_osc_value(AudioCompileCtx *audio,
                                               Ast *origin, AudioValue freq) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_saw_osc_kernel", "saw_osc.state",
      sizeof(SawOscState), __alignof__(SawOscState), lanes, args, 1);
}

static AudioValue audio_mir_emit_phasor_value(AudioCompileCtx *audio,
                                              Ast *origin, AudioValue freq) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_phasor_kernel", "phasor.state",
      sizeof(PhasorState), __alignof__(PhasorState), lanes, args, 1);
}

static AudioValue audio_mir_emit_phasor_sinc_value(AudioCompileCtx *audio,
                                                   Ast *origin, AudioValue freq,
                                                   AudioValue trig) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_phasor_kernel", "phasor.state",
      sizeof(PhasorState), __alignof__(PhasorState), lanes, args, 1);
}

static AudioValue audio_mir_emit_trig_value(AudioCompileCtx *audio, Ast *origin,
                                            AudioValue freq) {
  AudioValue values[] = {freq};
  AudioMirKernelArgLanes args[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  Type *params[] = {audio->ptr_double_type, &t_num, &t_num};
  size_t params_len = sizeof(params) / sizeof(params[0]);
  Type *kernel_type =
      audio_mir_fn_type(audio->arena, params, params_len, &t_num);
  if (!kernel_type) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    bool once = audio_mir_value_is_const_zero(audio, args[0].values[lane]);
    const char *kernel_symbol =
        once ? "ylc_audio_trig_once_kernel" : "ylc_audio_trig_kernel";
    const char *state_name = once ? "trig.once.state" : "trig.state";
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "%s.%d", state_name, lane)
                  : state_name;
    size_t state_size = once ? sizeof(TrigOnceState) : sizeof(TrigState);
    size_t state_align =
        once ? __alignof__(TrigOnceState) : __alignof__(TrigState);

    AudioStateSlot *slot = audio_mir_reserve_state_slot(
        audio, &t_num, state_size, state_align, slot_name);
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_num);
    MirValueId kernel_fn =
        audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
    MirValueId call_args[] = {state_ptr, audio->spf_param,
                              args[0].values[lane]};
    MirValueId sample = mir_call_value(b, &t_num, origin, kernel_fn,
                                       kernel_type, call_args, params_len);
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_pm_osc_values(AudioCompileCtx *audio,
                                               Ast *origin,
                                               AudioValue mod_ratio,
                                               AudioValue mod_index,
                                               AudioValue freq) {
  AudioValue values[] = {mod_index, mod_ratio, freq};
  AudioMirKernelArgLanes args[3];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, args, 3,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_pm_osc_kernel", "pm_osc.state",
      sizeof(PmOscState), __alignof__(PmOscState), lanes, args, 3);
}

static AudioValue audio_mir_emit_changed_value(AudioCompileCtx *audio,
                                               Ast *origin, AudioValue input) {
  AudioValue values[] = {input};
  return audio_mir_emit_num_state_kernel(
      audio, origin, "ylc_audio_changed_kernel", "changed.state",
      sizeof(ChangedState), __alignof__(ChangedState), values, 1);
}

static AudioValue audio_mir_emit_decay_values(AudioCompileCtx *audio,
                                              Ast *origin,
                                              AudioValue decay_time,
                                              AudioValue trig) {
  AudioValue values[] = {decay_time, trig};
  return audio_mir_emit_num_state_kernel(
      audio, origin, "ylc_audio_decay_kernel", "decay.state",
      sizeof(DecayState), __alignof__(DecayState), values, 2);
}

static AudioValue audio_mir_emit_scale_values(AudioCompileCtx *audio,
                                              Ast *origin, AudioValue lo,
                                              AudioValue hi, AudioValue value) {
  AudioValue values[] = {lo, hi, value};
  return audio_mir_emit_num_stateless_kernel(
      audio, origin, "ylc_audio_scale_kernel", values, 3);
}

static AudioValue audio_mir_emit_scale_bp_values(AudioCompileCtx *audio,
                                                 Ast *origin, AudioValue lo,
                                                 AudioValue hi,
                                                 AudioValue value) {
  AudioValue values[] = {lo, hi, value};
  return audio_mir_emit_num_stateless_kernel(
      audio, origin, "ylc_audio_scale_bp_kernel", values, 3);
}

static AudioValue audio_mir_emit_lfnoise_values(AudioCompileCtx *audio,
                                                Ast *origin, AudioValue freq,
                                                AudioValue lo, AudioValue hi) {
  AudioValue values[] = {freq, lo, hi};
  return audio_mir_emit_num_state_kernel(
      audio, origin, "ylc_audio_lfnoise_kernel", "lfnoise.state",
      sizeof(LfNoiseState), __alignof__(LfNoiseState), values, 3);
}

static AudioValue audio_mir_emit_lfnoise0_values(AudioCompileCtx *audio,
                                                 Ast *origin, AudioValue freq,
                                                 AudioValue lo, AudioValue hi) {
  AudioValue values[] = {freq, lo, hi};
  return audio_mir_emit_num_state_kernel(
      audio, origin, "ylc_audio_lfnoise0_kernel", "lfnoise0.state",
      sizeof(LfNoise0State), __alignof__(LfNoise0State), values, 3);
}

static AudioValue audio_mir_emit_adsr_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue attack, AudioValue decay,
    AudioValue sustain, AudioValue release, AudioValue trig) {
  AudioValue values[] = {attack, decay, sustain, release, trig};
  return audio_mir_emit_num_state_kernel(audio, origin, "ylc_audio_adsr_kernel",
                                         "adsr.state", sizeof(AdsrState),
                                         __alignof__(AdsrState), values, 5);
}

static AudioValue audio_mir_emit_rect_values(AudioCompileCtx *audio,
                                             Ast *origin, AudioValue duration,
                                             AudioValue trig) {
  AudioValue values[] = {duration, trig};
  return audio_mir_emit_num_state_kernel(audio, origin, "ylc_audio_rect_kernel",
                                         "rect.state", sizeof(RectState),
                                         __alignof__(RectState), values, 2);
}

static bool audio_mir_delay_state_size(AudioCompileCtx *audio,
                                       AudioValue max_delay_value,
                                       const char *name,
                                       size_t *out_state_size,
                                       int32_t *out_max_samples) {
  if (!audio || !out_state_size || !out_max_samples ||
      audio_mir_value_lane_count(max_delay_value) != 1) {
    fprintf(stderr, "Error: %s expects a constant scalar max delay\n",
            name ? name : "delay");
    return false;
  }

  double max_delay_secs = 0.0;
  if (!audio_mir_value_const_double(audio, max_delay_value.value,
                                    &max_delay_secs) ||
      !isfinite(max_delay_secs) || max_delay_secs <= 0.0) {
    fprintf(stderr, "Error: %s max delay must be a constant > 0\n",
            name ? name : "delay");
    return false;
  }

  int sample_rate = ctx_sample_rate();
  if (sample_rate <= 0) {
    sample_rate = 48000;
  }
  double samples_f = ceil(max_delay_secs * (double)sample_rate) + 2.0;
  if (!isfinite(samples_f) || samples_f > (double)INT_MAX) {
    fprintf(stderr, "Error: %s max delay is too large\n",
            name ? name : "delay");
    return false;
  }

  int32_t max_samples = (int32_t)samples_f;
  if (max_samples < 2) {
    max_samples = 2;
  }

  size_t sample_count = (size_t)max_samples;
  if (sample_count >
      (SIZE_MAX - sizeof(DelayLineState)) / sizeof(double)) {
    fprintf(stderr, "Error: %s delay state is too large\n",
            name ? name : "delay");
    return false;
  }

  size_t state_size =
      sizeof(DelayLineState) + (sample_count * sizeof(double));
  if (state_size > (size_t)INT_MAX) {
    fprintf(stderr, "Error: %s delay state is too large\n",
            name ? name : "delay");
    return false;
  }
  *out_state_size = audio_mir_align_up(state_size, 8);
  *out_max_samples = max_samples;
  return true;
}

static AudioValue audio_mir_emit_delay_line_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue delay_secs,
    AudioValue max_delay_secs, AudioValue feedback, AudioValue input,
    const char *kernel_symbol, const char *state_name,
    uint64_t control_expand_mask) {
  if (!audio || !origin || !kernel_symbol || !state_name) {
    return AUDIO_VALUE_NULL;
  }

  size_t state_size = 0;
  int32_t max_samples = 0;
  if (!audio_mir_delay_state_size(audio, max_delay_secs, state_name,
                                  &state_size, &max_samples)) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue controls[] = {delay_secs, feedback, input};
  AudioMirKernelArgLanes control_args[3];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, controls, control_args, 3, control_expand_mask, 0,
          &lanes)) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {audio->ptr_char_type, &t_num, &t_int,
                    &t_num,           &t_num, &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (!kernel_type || kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "%s.%d", state_name, lane)
                  : state_name;
    AudioStateSlot *slot =
        audio_mir_reserve_state_block(audio, state_size, 8, slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_char);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {
        state_ptr,
        audio->spf_param,
        mir_const_int(b, &t_int, origin, max_samples),
        control_args[0].values[lane],
        control_args[1].values[lane],
        control_args[2].values[lane],
    };
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_kill_on_end_value(AudioCompileCtx *audio,
                                                   Ast *origin,
                                                   AudioValue signal) {
  AudioValue values[] = {signal};
  AudioMirKernelArgLanes signal_arg[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, values, signal_arg, 1,
                                           &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *node_values = audio_mir_alloc_lane_values(audio, lanes);
  if (!node_values) {
    return AUDIO_VALUE_NULL;
  }
  for (int lane = 0; lane < lanes; lane++) {
    node_values[lane] = audio->node_param;
  }

  AudioMirKernelArgLanes args[] = {
      {.type = &t_ptr, .values = node_values},
      signal_arg[0],
  };
  return audio_mir_emit_kernel_call_lanes(
      audio, origin, "ylc_audio_kill_on_end_kernel", "kill_on_end.state",
      sizeof(KillOnEndState), __alignof__(KillOnEndState), lanes, args, 2);
}

static AudioValue audio_mir_emit_tabread_values(AudioCompileCtx *audio,
                                                Ast *origin, AudioValue table,
                                                AudioValue phase,
                                                const char *kernel_symbol) {
  if (!audio || !kernel_symbol || table.value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue phase_values[] = {phase};
  AudioMirKernelArgLanes phase_arg[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args(audio, origin, phase_values,
                                           phase_arg, 1, &lanes)) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  MirValueId size = mir_tuple_get(b, &t_int, origin, table.value, 0);
  MirValueId offset = mir_tuple_get(b, &t_int, origin, table.value, 1);
  MirValueId data =
      mir_tuple_get(b, audio->ptr_double_type, origin, table.value, 2);
  if (size == MIR_NO_VALUE || offset == MIR_NO_VALUE || data == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  Type *params[] = {audio->ptr_double_type, &t_num, &t_int, &t_int,
                    audio->ptr_double_type, &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  MirValueId state_arg = audio_mir_state_base_arg(audio, origin);

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId args[] = {state_arg, audio->spf_param,         size, offset,
                         data,      phase_arg[0].values[lane]};
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, args,
                       sizeof(args) / sizeof(args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_bufplay_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue buffer, AudioValue rate,
    AudioValue start_pos, AudioValue trig, const char *kernel_symbol,
    uint64_t control_expand_mask) {
  if (!audio || !origin || !kernel_symbol || buffer.value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue controls[] = {rate, start_pos, trig};
  AudioMirKernelArgLanes control_args[3];
  int control_lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, controls, control_args, 3, control_expand_mask, 0,
          &control_lanes)) {
    return AUDIO_VALUE_NULL;
  }

  int lanes = control_lanes;
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {audio->ptr_double_type, &t_num, &t_int, &t_int,
                    audio->ptr_double_type, &t_num, &t_num, &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  if (!kernel_type) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId buffer_value = audio_mir_value_lane(buffer, 0);
    if (buffer_value == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId size = mir_tuple_get(b, &t_int, origin, buffer_value, 0);
    MirValueId offset = mir_tuple_get(b, &t_int, origin, buffer_value, 1);
    MirValueId data =
        mir_tuple_get(b, audio->ptr_double_type, origin, buffer_value, 2);
    if (size == MIR_NO_VALUE || offset == MIR_NO_VALUE ||
        data == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "bufplay.state.%d", lane)
                  : "bufplay.state";
    AudioStateSlot *slot =
        audio_mir_reserve_state_slot(audio, &t_num, sizeof(BufplayState),
                                     __alignof__(BufplayState), slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_num);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {state_ptr,
                              audio->spf_param,
                              size,
                              offset,
                              data,
                              control_args[0].values[lane],
                              control_args[1].values[lane],
                              control_args[2].values[lane]};
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_mbufplay_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue channel_count,
    AudioValue buffer, AudioValue rate, AudioValue start_pos, AudioValue trig,
    const char *kernel_symbol, uint64_t control_expand_mask) {
  if (!kernel_symbol) {
    return AUDIO_VALUE_NULL;
  }

  if (audio_mir_value_lane_count(channel_count) != 1) {
    fprintf(stderr,
            "Error: mbufplay expects a constant scalar channel count\n");
    return AUDIO_VALUE_NULL;
  }

  int channels = 0;
  if (!audio_mir_value_const_int(audio, channel_count.value, &channels)) {
    fprintf(stderr,
            "Error: mbufplay expects a constant integer channel count\n");
    return AUDIO_VALUE_NULL;
  }
  if (channels <= 0) {
    fprintf(stderr, "Error: mbufplay channel count must be > 0\n");
    return AUDIO_VALUE_NULL;
  }

  if (buffer.value == MIR_NO_VALUE || audio_mir_value_lane_count(buffer) <= 0) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue controls[] = {rate, start_pos, trig};
  AudioMirKernelArgLanes control_args[3];
  int control_lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, controls, control_args, 3, control_expand_mask,
          channels, &control_lanes)) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {
      audio->ptr_double_type, &t_num, &t_int, &t_int, &t_int, &t_int,
      audio->ptr_double_type, &t_num, &t_num, &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  if (!kernel_type) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId buffer_value = audio_mir_value_lane(buffer, 0);
  if (buffer_value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }
  MirValueId size = mir_tuple_get(b, &t_int, origin, buffer_value, 0);
  MirValueId offset = mir_tuple_get(b, &t_int, origin, buffer_value, 1);
  MirValueId data =
      mir_tuple_get(b, audio->ptr_double_type, origin, buffer_value, 2);
  if (size == MIR_NO_VALUE || offset == MIR_NO_VALUE || data == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples = audio_mir_alloc_lane_values(audio, channels);
  if (!samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < channels; lane++) {
    const char *slot_name =
        channels > 1 ? mir_arena_printf(audio->arena, "mbufplay.state.%d", lane)
                     : "mbufplay.state";
    AudioStateSlot *slot =
        audio_mir_reserve_state_slot(audio, &t_num, sizeof(BufplayState),
                                     __alignof__(BufplayState), slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_num);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {
        state_ptr,
        audio->spf_param,
        mir_const_int(b, &t_int, origin, channels),
        mir_const_int(b, &t_int, origin, lane),
        size,
        offset,
        data,
        control_args[0].values[lane],
        control_args[1].values[lane],
        control_args[2].values[lane],
    };
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (channels == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, channels);
}

typedef struct AudioMirArrayParts {
  MirValueId size;
  MirValueId offset;
  MirValueId data;
} AudioMirArrayParts;

static bool audio_mir_extract_array_parts(AudioCompileCtx *audio, Ast *origin,
                                          AudioValue array_value,
                                          AudioMirArrayParts *out) {
  if (!audio || !audio->kernel_builder || !out ||
      audio_mir_value_lane_count(array_value) <= 0) {
    return false;
  }

  MirBuilder *b = audio->kernel_builder;
  MirValueId value = audio_mir_value_lane(array_value, 0);
  if (value == MIR_NO_VALUE) {
    return false;
  }

  MirValueId size = mir_tuple_get(b, &t_int, origin, value, 0);
  MirValueId offset = mir_tuple_get(b, &t_int, origin, value, 1);
  MirValueId data = mir_tuple_get(b, audio->ptr_double_type, origin, value, 2);
  if (size == MIR_NO_VALUE || offset == MIR_NO_VALUE ||
      data == MIR_NO_VALUE) {
    return false;
  }

  *out = (AudioMirArrayParts){
      .size = size,
      .offset = offset,
      .data = data,
  };
  return true;
}

static AudioValue audio_mir_emit_array_trigger_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue array, AudioValue trig,
    const char *kernel_symbol, const char *state_name, size_t state_size,
    size_t state_align, uint64_t trig_expand_mask) {
  if (!audio || !origin || !kernel_symbol || !state_name) {
    return AUDIO_VALUE_NULL;
  }

  AudioMirArrayParts parts = {0};
  if (!audio_mir_extract_array_parts(audio, origin, array, &parts)) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue trig_values[] = {trig};
  AudioMirKernelArgLanes trig_arg[1];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, trig_values, trig_arg, 1, trig_expand_mask, 0,
          &lanes)) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {audio->ptr_char_type, &t_num, &t_int,
                    &t_int,           audio->ptr_double_type, &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (!kernel_type || kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "%s.%d", state_name, lane)
                  : state_name;
    AudioStateSlot *slot =
        audio_mir_reserve_state_block(audio, state_size, state_align,
                                      slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_char);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {state_ptr,
                              audio->spf_param,
                              parts.size,
                              parts.offset,
                              parts.data,
                              trig_arg[0].values[lane]};
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static bool audio_mir_grains_max_grains(AudioCompileCtx *audio,
                                        AudioValue value, const char *name,
                                        int *out_max_grains) {
  if (!audio || !out_max_grains ||
      audio_mir_value_lane_count(value) != 1) {
    fprintf(stderr, "Error: %s expects a constant scalar max grain count\n",
            name ? name : "grains");
    return false;
  }

  int max_grains = 0;
  if (!audio_mir_value_const_int(audio, value.value, &max_grains)) {
    fprintf(stderr, "Error: %s expects a constant integer max grain count\n",
            name ? name : "grains");
    return false;
  }
  if (max_grains <= 0) {
    fprintf(stderr, "Error: %s max grain count must be > 0\n",
            name ? name : "grains");
    return false;
  }

  *out_max_grains = max_grains;
  return true;
}

static bool audio_mir_grains_state_size(int max_grains, size_t *out_size) {
  if (max_grains <= 0 || !out_size) {
    return false;
  }

  const size_t header = sizeof(GrainOscState);
  const size_t per_grain = (sizeof(double) * 5) + sizeof(int32_t);
  size_t grain_count = (size_t)max_grains;
  if (grain_count > (SIZE_MAX - header) / per_grain) {
    return false;
  }

  size_t size = header + (grain_count * per_grain);
  *out_size = audio_mir_align_up(size, 8);
  return true;
}

static AudioValue audio_mir_emit_grains_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue max_grains_value,
    AudioValue buffer, AudioValue rate, AudioValue position, AudioValue width,
    AudioValue trig, const char *kernel_symbol, uint64_t control_expand_mask) {
  if (!audio || !origin || !kernel_symbol) {
    return AUDIO_VALUE_NULL;
  }

  int max_grains = 0;
  if (!audio_mir_grains_max_grains(audio, max_grains_value, "grains",
                                   &max_grains)) {
    return AUDIO_VALUE_NULL;
  }

  size_t state_size = 0;
  if (!audio_mir_grains_state_size(max_grains, &state_size) ||
      state_size > (size_t)INT_MAX) {
    fprintf(stderr, "Error: grains state is too large\n");
    return AUDIO_VALUE_NULL;
  }

  AudioMirArrayParts source = {0};
  if (!audio_mir_extract_array_parts(audio, origin, buffer, &source)) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue controls[] = {rate, position, width, trig};
  AudioMirKernelArgLanes control_args[4];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, controls, control_args, 4, control_expand_mask, 0,
          &lanes)) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {audio->ptr_char_type,
                    &t_num,
                    &t_int,
                    &t_int,
                    &t_int,
                    audio->ptr_double_type,
                    &t_num,
                    &t_num,
                    &t_num,
                    &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (!kernel_type || kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "grains.state.%d", lane)
                  : "grains.state";
    AudioStateSlot *slot =
        audio_mir_reserve_state_block(audio, state_size, 8, slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_char);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {state_ptr,
                              audio->spf_param,
                              mir_const_int(b, &t_int, origin, max_grains),
                              source.size,
                              source.offset,
                              source.data,
                              control_args[0].values[lane],
                              control_args[1].values[lane],
                              control_args[2].values[lane],
                              control_args[3].values[lane]};
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_grains_env_values(
    AudioCompileCtx *audio, Ast *origin, AudioValue max_grains_value,
    AudioValue buffer, AudioValue env_buffer, AudioValue rate,
    AudioValue position, AudioValue width, AudioValue trig,
    const char *kernel_symbol, uint64_t control_expand_mask) {
  if (!audio || !origin || !kernel_symbol) {
    return AUDIO_VALUE_NULL;
  }

  int max_grains = 0;
  if (!audio_mir_grains_max_grains(audio, max_grains_value, "grains_env",
                                   &max_grains)) {
    return AUDIO_VALUE_NULL;
  }

  size_t state_size = 0;
  if (!audio_mir_grains_state_size(max_grains, &state_size) ||
      state_size > (size_t)INT_MAX) {
    fprintf(stderr, "Error: grains_env state is too large\n");
    return AUDIO_VALUE_NULL;
  }

  AudioMirArrayParts source = {0};
  AudioMirArrayParts envelope = {0};
  if (!audio_mir_extract_array_parts(audio, origin, buffer, &source) ||
      !audio_mir_extract_array_parts(audio, origin, env_buffer, &envelope)) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue controls[] = {rate, position, width, trig};
  AudioMirKernelArgLanes control_args[4];
  int lanes = 0;
  if (!audio_mir_normalize_num_kernel_args_masked(
          audio, origin, controls, control_args, 4, control_expand_mask, 0,
          &lanes)) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *params[] = {audio->ptr_char_type,
                    &t_num,
                    &t_int,
                    &t_int,
                    &t_int,
                    audio->ptr_double_type,
                    &t_int,
                    &t_int,
                    audio->ptr_double_type,
                    &t_num,
                    &t_num,
                    &t_num,
                    &t_num};
  Type *kernel_type = audio_mir_fn_type(
      audio->arena, params, sizeof(params) / sizeof(params[0]), &t_num);
  MirValueId kernel_fn =
      audio_mir_extern_ref(b, kernel_symbol, kernel_type, origin);
  if (!kernel_type || kernel_fn == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    const char *slot_name =
        lanes > 1 ? mir_arena_printf(audio->arena, "grains_env.state.%d", lane)
                  : "grains_env.state";
    AudioStateSlot *slot =
        audio_mir_reserve_state_block(audio, state_size, 8, slot_name);
    if (!slot) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId state_ptr = audio_mir_state_slot_ptr(
        audio, b, origin, audio->state_param, slot->offset, &t_char);
    if (state_ptr == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    MirValueId call_args[] = {state_ptr,
                              audio->spf_param,
                              mir_const_int(b, &t_int, origin, max_grains),
                              source.size,
                              source.offset,
                              source.data,
                              envelope.size,
                              envelope.offset,
                              envelope.data,
                              control_args[0].values[lane],
                              control_args[1].values[lane],
                              control_args[2].values[lane],
                              control_args[3].values[lane]};
    MirValueId sample =
        mir_call_value(b, &t_num, origin, kernel_fn, kernel_type, call_args,
                       sizeof(call_args) / sizeof(call_args[0]));
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_value(&t_num, sample, 1);
    }
    samples[lane] = sample;
  }

  return audio_mir_multi_value(audio, origin, origin->type, samples, lanes);
}

static AudioValue audio_mir_emit_array_of_buf_value(AudioCompileCtx *audio,
                                                    Ast *origin,
                                                    AudioValue node_value) {
  if (!audio || node_value.value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *node_type = node_value.type ? node_value.type : &t_ptr;
  MirValueId node =
      audio_mir_value_as_node(b, origin, node_value.value, node_type);

  Type *size_params[] = {&t_ptr};
  Type *size_type =
      audio_mir_fn_type(audio->arena, size_params,
                        sizeof(size_params) / sizeof(size_params[0]), &t_int);
  MirValueId size_fn =
      audio_mir_extern_ref(b, "ylc_audio_bufsize", size_type, origin);
  MirValueId size = mir_call_value(b, &t_int, origin, size_fn, size_type,
                                   (MirValueId[]){node}, 1);

  Type *data_params[] = {&t_ptr};
  Type *data_type = audio_mir_fn_type(
      audio->arena, data_params, sizeof(data_params) / sizeof(data_params[0]),
      audio->ptr_double_type);
  MirValueId data_fn =
      audio_mir_extern_ref(b, "ylc_get_output_buf", data_type, origin);
  MirValueId data = mir_call_value(b, audio->ptr_double_type, origin, data_fn,
                                   data_type, (MirValueId[]){node}, 1);
  if (size == MIR_NO_VALUE || data == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirValueIdVec fields = {0};
  mir_value_id_vec_push(audio->arena, &fields, size);
  mir_value_id_vec_push(audio->arena, &fields,
                        mir_const_int(b, &t_int, origin, 0));
  mir_value_id_vec_push(audio->arena, &fields, data);
  MirValueId array = mir_tuple(b, origin->type ? origin->type : node_value.type,
                               origin, fields);
  return audio_mir_value(origin->type ? origin->type : node_value.type, array,
                         1);
}

static AudioValue audio_mir_emit_bufsize_value(AudioCompileCtx *audio,
                                               Ast *origin,
                                               AudioValue node_value) {
  if (!audio || node_value.value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *b = audio->kernel_builder;
  Type *node_type = node_value.type ? node_value.type : &t_ptr;
  MirValueId node =
      audio_mir_value_as_node(b, origin, node_value.value, node_type);
  Type *params[] = {&t_ptr};
  Type *type = audio_mir_fn_type(audio->arena, params,
                                 sizeof(params) / sizeof(params[0]), &t_int);
  MirValueId fn = audio_mir_extern_ref(b, "ylc_audio_bufsize", type, origin);
  MirValueId size =
      mir_call_value(b, &t_int, origin, fn, type, (MirValueId[]){node}, 1);
  return audio_mir_value(&t_int, size, 1);
}

typedef struct AudioBuiltinArgSelection {
  const AudioValue *values;
  size_t argc;
  uint64_t lane_expand_mask;
} AudioBuiltinArgSelection;

static bool audio_builtin_select_args(AudioCompileCtx *audio,
                                      const AudioBuiltin *builtin,
                                      AudioValue *args, size_t argc,
                                      AudioBuiltinArgSelection *out) {
  if (!audio || !builtin || (argc && !args) || !out) {
    return false;
  }

  size_t kernel_argc = builtin->kernel_argc ? builtin->kernel_argc : argc;
  const bool needs_copy = builtin->arg_order || kernel_argc != argc;
  AudioValue *values =
      needs_copy
          ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * kernel_argc,
                            __alignof__(AudioValue))
          : args;
  if (kernel_argc && !values) {
    return false;
  }

  uint64_t lane_expand_mask = 0;
  for (size_t i = 0; i < kernel_argc; i++) {
    size_t source_index = builtin->arg_order ? builtin->arg_order[i] : i;
    if (source_index >= argc) {
      return false;
    }
    if (needs_copy) {
      values[i] = args[source_index];
    }
    if (builtin->lane_expand_mask & audio_arg_mask(source_index)) {
      lane_expand_mask |= audio_arg_mask(i);
    }
  }

  *out = (AudioBuiltinArgSelection){
      .values = values,
      .argc = kernel_argc,
      .lane_expand_mask = lane_expand_mask,
  };
  return true;
}

static AudioValue audio_builtin_emit_num_state(const AudioBuiltin *builtin,
                                               AudioCompileCtx *audio,
                                               Ast *origin, AudioValue *args,
                                               size_t argc) {
  if (!builtin || !builtin->kernel_symbol || !builtin->state_name) {
    return AUDIO_VALUE_NULL;
  }

  AudioBuiltinArgSelection selection = {0};
  if (!audio_builtin_select_args(audio, builtin, args, argc, &selection)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_num_state_kernel_masked(
      audio, origin, builtin->kernel_symbol, builtin->state_name,
      builtin->state_size, builtin->state_align, selection.values,
      selection.argc, selection.lane_expand_mask);
}

static AudioValue audio_builtin_emit_num_stateless(const AudioBuiltin *builtin,
                                                   AudioCompileCtx *audio,
                                                   Ast *origin,
                                                   AudioValue *args,
                                                   size_t argc) {
  if (!builtin || !builtin->kernel_symbol) {
    return AUDIO_VALUE_NULL;
  }

  AudioBuiltinArgSelection selection = {0};
  if (!audio_builtin_select_args(audio, builtin, args, argc, &selection)) {
    return AUDIO_VALUE_NULL;
  }

  return audio_mir_emit_num_stateless_kernel_masked(
      audio, origin, builtin->kernel_symbol, selection.values, selection.argc,
      selection.lane_expand_mask);
}

static AudioValue audio_builtin_emit_trig(const AudioBuiltin *builtin,
                                          AudioCompileCtx *audio, Ast *origin,
                                          AudioValue *args, size_t argc) {
  (void)builtin;
  return argc == 1 ? audio_mir_emit_trig_value(audio, origin, args[0])
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_builtin_emit_kill_on_end(const AudioBuiltin *builtin,
                                                 AudioCompileCtx *audio,
                                                 Ast *origin, AudioValue *args,
                                                 size_t argc) {
  (void)builtin;
  return argc == 1 ? audio_mir_emit_kill_on_end_value(audio, origin, args[0])
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_builtin_emit_tabread(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc) {
  return argc == 2 ? audio_mir_emit_tabread_values(
                         audio, origin, args[0], args[1],
                         builtin ? builtin->kernel_symbol : NULL)
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_builtin_emit_tabread_samp(const AudioBuiltin *builtin,
                                                  AudioCompileCtx *audio,
                                                  Ast *origin, AudioValue *args,
                                                  size_t argc) {
  return argc == 2 ? audio_mir_emit_tabread_values(
                         audio, origin, args[0], args[1],
                         builtin ? builtin->kernel_symbol : NULL)
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_builtin_emit_bufplay(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc) {
  if (!builtin || argc != 4) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t control_mask = 0;
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(1)) {
    control_mask |= AUDIO_ARG_MASK(0);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(2)) {
    control_mask |= AUDIO_ARG_MASK(1);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(3)) {
    control_mask |= AUDIO_ARG_MASK(2);
  }

  return audio_mir_emit_bufplay_values(audio, origin, args[0], args[1], args[2],
                                       args[3], builtin->kernel_symbol,
                                       control_mask);
}

static AudioValue audio_builtin_emit_mbufplay(const AudioBuiltin *builtin,
                                              AudioCompileCtx *audio,
                                              Ast *origin, AudioValue *args,
                                              size_t argc) {
  if (!builtin || argc != 5) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t control_mask = 0;
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(2)) {
    control_mask |= AUDIO_ARG_MASK(0);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(3)) {
    control_mask |= AUDIO_ARG_MASK(1);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(4)) {
    control_mask |= AUDIO_ARG_MASK(2);
  }

  return audio_mir_emit_mbufplay_values(audio, origin, args[0], args[1],
                                        args[2], args[3], args[4],
                                        builtin->kernel_symbol, control_mask);
}

static AudioValue audio_builtin_emit_delay_line(const AudioBuiltin *builtin,
                                                AudioCompileCtx *audio,
                                                Ast *origin, AudioValue *args,
                                                size_t argc) {
  if (!builtin || argc != 4) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t control_mask = 0;
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(0)) {
    control_mask |= AUDIO_ARG_MASK(0);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(2)) {
    control_mask |= AUDIO_ARG_MASK(1);
  }
  if (builtin->lane_expand_mask & AUDIO_ARG_MASK(3)) {
    control_mask |= AUDIO_ARG_MASK(2);
  }

  return audio_mir_emit_delay_line_values(
      audio, origin, args[0], args[1], args[2], args[3],
      builtin->kernel_symbol, builtin->state_name, control_mask);
}

static AudioValue audio_builtin_emit_array_trigger(const AudioBuiltin *builtin,
                                                   AudioCompileCtx *audio,
                                                   Ast *origin,
                                                   AudioValue *args,
                                                   size_t argc) {
  if (!builtin || argc != 2) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t trig_mask = (builtin->lane_expand_mask & AUDIO_ARG_MASK(1))
                           ? AUDIO_ARG_MASK(0)
                           : 0;
  return audio_mir_emit_array_trigger_values(
      audio, origin, args[0], args[1], builtin->kernel_symbol,
      builtin->state_name, builtin->state_size, builtin->state_align,
      trig_mask);
}

static AudioValue audio_builtin_emit_grains(const AudioBuiltin *builtin,
                                            AudioCompileCtx *audio,
                                            Ast *origin, AudioValue *args,
                                            size_t argc) {
  if (!builtin || argc != 6) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t control_mask = 0;
  for (size_t i = 2; i < 6; i++) {
    if (builtin->lane_expand_mask & AUDIO_ARG_MASK(i)) {
      control_mask |= AUDIO_ARG_MASK(i - 2);
    }
  }

  return audio_mir_emit_grains_values(
      audio, origin, args[0], args[1], args[2], args[3], args[4], args[5],
      builtin->kernel_symbol, control_mask);
}

static AudioValue audio_builtin_emit_grains_env(const AudioBuiltin *builtin,
                                                AudioCompileCtx *audio,
                                                Ast *origin, AudioValue *args,
                                                size_t argc) {
  if (!builtin || argc != 7) {
    return AUDIO_VALUE_NULL;
  }

  uint64_t control_mask = 0;
  for (size_t i = 3; i < 7; i++) {
    if (builtin->lane_expand_mask & AUDIO_ARG_MASK(i)) {
      control_mask |= AUDIO_ARG_MASK(i - 3);
    }
  }

  return audio_mir_emit_grains_env_values(
      audio, origin, args[0], args[1], args[2], args[3], args[4], args[5],
      args[6], builtin->kernel_symbol, control_mask);
}

static AudioValue audio_builtin_emit_array_of_buf(const AudioBuiltin *builtin,
                                                  AudioCompileCtx *audio,
                                                  Ast *origin, AudioValue *args,
                                                  size_t argc) {
  (void)builtin;
  return argc == 1 ? audio_mir_emit_array_of_buf_value(audio, origin, args[0])
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_builtin_emit_bufsize(const AudioBuiltin *builtin,
                                             AudioCompileCtx *audio,
                                             Ast *origin, AudioValue *args,
                                             size_t argc) {
  (void)builtin;
  return argc == 1 ? audio_mir_emit_bufsize_value(audio, origin, args[0])
                   : AUDIO_VALUE_NULL;
}

static AudioValue audio_mir_emit_builtin_values(AudioCompileCtx *audio,
                                                Ast *origin, const char *name,
                                                AudioValue *args, size_t argc) {
  if (!audio || !name || (argc && !args)) {
    return AUDIO_VALUE_NULL;
  }

  const AudioBuiltin *builtin = audio_mir_find_builtin(name);
  if (!builtin || !builtin->emit) {
    return AUDIO_VALUE_NULL;
  }

  int expected_argc = audio_mir_builtin_arity(audio, name);
  if (expected_argc <= 0 || argc != (size_t)expected_argc) {
    return AUDIO_VALUE_NULL;
  }

  return builtin->emit(builtin, audio, origin, args, argc);
}

static AudioValue audio_mir_make_partial_builtin(AudioCompileCtx *audio,
                                                 Ast *app, const char *name) {
  int arity = audio_mir_builtin_arity(audio, name);
  size_t argc = app ? app->data.AST_APPLICATION.len : 0;
  if (!audio || !app || arity <= 0 || argc >= (size_t)arity) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue *args = mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                                     __alignof__(AudioValue));
  if (argc && !args) {
    return AUDIO_VALUE_NULL;
  }

  for (size_t i = 0; i < argc; i++) {
    args[i] = audio_mir_expr(audio, app->data.AST_APPLICATION.args + i);
    if (!audio_mir_value_is_valid(args[i])) {
      return AUDIO_VALUE_NULL;
    }
  }

  AudioPartialBuiltin *partial =
      mir_arena_alloc(audio->arena, sizeof(AudioPartialBuiltin),
                      __alignof__(AudioPartialBuiltin));
  if (!partial) {
    return AUDIO_VALUE_NULL;
  }
  *partial = (AudioPartialBuiltin){
      .name = name,
      .type = app->type,
      .argc = argc,
      .args = args,
  };
  return (AudioValue){
      .kind = AUDIO_VALUE_PARTIAL_BUILTIN,
      .type = app->type,
      .value = MIR_NO_VALUE,
      .lanes = 0,
      .vec = NULL,
      .synth = NULL,
      .partial_synth = NULL,
      .partial = partial,
  };
}

static AudioValue
audio_mir_apply_partial_builtin(AudioCompileCtx *audio, Ast *app,
                                AudioPartialBuiltin *partial) {
  if (!audio || !app || !partial || !partial->name) {
    return AUDIO_VALUE_NULL;
  }

  int arity = audio_mir_builtin_arity(audio, partial->name);
  size_t new_argc = app->data.AST_APPLICATION.len;
  size_t total_argc = partial->argc + new_argc;
  if (arity <= 0 || total_argc > (size_t)arity) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue *args = mir_arena_alloc(
      audio->arena, sizeof(AudioValue) * total_argc, __alignof__(AudioValue));
  if (total_argc && !args) {
    return AUDIO_VALUE_NULL;
  }

  for (size_t i = 0; i < partial->argc; i++) {
    args[i] = partial->args[i];
  }
  for (size_t i = 0; i < new_argc; i++) {
    args[partial->argc + i] =
        audio_mir_expr(audio, app->data.AST_APPLICATION.args + i);
    if (!audio_mir_value_is_valid(args[partial->argc + i])) {
      return AUDIO_VALUE_NULL;
    }
  }

  if (total_argc < (size_t)arity) {
    AudioPartialBuiltin *extended =
        mir_arena_alloc(audio->arena, sizeof(AudioPartialBuiltin),
                        __alignof__(AudioPartialBuiltin));
    if (!extended) {
      return AUDIO_VALUE_NULL;
    }
    *extended = (AudioPartialBuiltin){
        .name = partial->name,
        .type = app->type,
        .argc = total_argc,
        .args = args,
    };
    return (AudioValue){
        .kind = AUDIO_VALUE_PARTIAL_BUILTIN,
        .type = app->type,
        .value = MIR_NO_VALUE,
        .lanes = 0,
        .vec = NULL,
        .synth = NULL,
        .partial_synth = NULL,
        .partial = extended,
    };
  }

  return audio_mir_emit_builtin_values(audio, app, partial->name, args,
                                       total_argc);
}

static AudioValue audio_mir_make_partial_synth(AudioCompileCtx *audio, Ast *app,
                                               MirAudioSynthSymbol *synth) {
  if (!audio || !app || !synth) {
    return AUDIO_VALUE_NULL;
  }

  size_t argc = audio_mir_application_value_arg_count(app);
  if (argc >= (size_t)synth->num_inputs) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue *args =
      argc ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                             __alignof__(AudioValue))
           : NULL;
  if (argc && !args) {
    return AUDIO_VALUE_NULL;
  }

  for (size_t i = 0; i < argc; i++) {
    args[i] = audio_mir_expr(audio, app->data.AST_APPLICATION.args + i);
    if (!audio_mir_value_is_valid(args[i])) {
      return AUDIO_VALUE_NULL;
    }
  }

  AudioPartialSynth *partial = mir_arena_alloc(
      audio->arena, sizeof(AudioPartialSynth), __alignof__(AudioPartialSynth));
  if (!partial) {
    return AUDIO_VALUE_NULL;
  }
  *partial = (AudioPartialSynth){
      .synth = synth,
      .type = app->type,
      .argc = argc,
      .args = args,
  };
  return audio_mir_partial_synth_value(app->type, partial);
}

static MirAudioSynthSymbol *
audio_mir_make_synth_symbol(MirArena *arena, MirAudioSynthBuildCtx *ctx,
                            AudioSynthScope scope) {
  if (!arena || !ctx || !ctx->name || !ctx->ctor_fn || !ctx->init_fn ||
      !ctx->kernel_fn) {
    return NULL;
  }

  MirAudioSynthSymbol *synth = mir_arena_alloc(
      arena, sizeof(MirAudioSynthSymbol), __alignof__(MirAudioSynthSymbol));
  if (!synth) {
    return NULL;
  }

  *synth = (MirAudioSynthSymbol){
      .name = ctx->name,
      .scope = scope,
      .num_inputs = ctx->num_inputs,
      .capture_asts = ctx->capture_asts,
      .capture_count = ctx->capture_count,
      .state_bytes = ctx->state_bytes,
      .ctor_fn = ctx->ctor_fn,
      .init_fn = ctx->init_fn,
      .kernel_fn = ctx->kernel_fn,
      .frame_fn = ctx->frame_fn,
  };
  return synth;
}

static MirAudioSynthSymbol *audio_mir_create_local_synth(AudioCompileCtx *audio,
                                                         MirCtx *mir_ctx,
                                                         const char *local_name,
                                                         Ast *lambda) {
  if (!audio || !audio->bundle || !audio->program || !audio->arena ||
      !mir_ctx || !local_name || !lambda || lambda->tag != AST_LAMBDA) {
    return NULL;
  }

  size_t capture_count = 0;
  Ast **captures =
      audio_mir_collect_lambda_captures(audio->arena, lambda, &capture_count);
  if (capture_count > 0 && !captures) {
    return NULL;
  }

  const char *parent_name =
      audio->bundle->name ? audio->bundle->name : "audio_synth";
  const char *scoped_name =
      mir_arena_printf(audio->arena, "%s.%s", parent_name, local_name);
  MirAudioSynthBuildCtx local_ctx = {
      .program = audio->program,
      .arena = audio->arena,
      .app = lambda,
      .lambda = lambda,
      .parent_ctx = mir_ctx,
      .name = scoped_name,
      .num_inputs = audio_mir_lambda_input_count(lambda),
      .capture_asts = captures,
      .capture_count = capture_count,
      .state_bytes = 0,
  };

  if (!audio_mir_build_local_synth_functions(&local_ctx)) {
    fprintf(stderr, "audio_jit: failed to build local synth `%s`\n",
            local_name);
    return NULL;
  }

  audio_mir_emit_local_synth_stubs(&local_ctx);

  MirAudioSynthSymbol *synth =
      audio_mir_make_synth_symbol(audio->arena, &local_ctx, AUDIO_SYNTH_LOCAL);
  return synth;
}

static bool audio_mir_bind_local_synth(AudioCompileCtx *audio, MirCtx *mir_ctx,
                                       Ast *binding, Ast *lambda) {
  if (!audio || !audio->bundle || !audio->program || !audio->arena ||
      !mir_ctx || !binding || !lambda || lambda->tag != AST_LAMBDA) {
    return false;
  }
  if (binding->tag == AST_PLACEHOLDER_ID) {
    return true;
  }
  if (binding->tag != AST_IDENTIFIER || !binding->data.AST_IDENTIFIER.value) {
    return false;
  }
  if (binding->data.AST_IDENTIFIER.length == 1 &&
      binding->data.AST_IDENTIFIER.value[0] == '_') {
    return true;
  }

  const char *local_name = binding->data.AST_IDENTIFIER.value;
  MirAudioSynthSymbol *synth =
      audio_mir_create_local_synth(audio, mir_ctx, local_name, lambda);
  if (!synth) {
    return false;
  }

  return mir_ctx_bind_custom_symbol(mir_ctx, local_name, lambda->type, lambda,
                                    MirAudioSynthSymbolHandler, synth);
}

typedef enum AudioStaticValueKind {
  AUDIO_STATIC_NONE,
  AUDIO_STATIC_INT,
  AUDIO_STATIC_DOUBLE,
  AUDIO_STATIC_TUPLE,
  AUDIO_STATIC_LIST,
  AUDIO_STATIC_ARRAY_LITERAL,
} AudioStaticValueKind;

typedef struct AudioStaticValue {
  AudioStaticValueKind kind;
  Ast *ast;
  Type *type;
  size_t len;
  struct AudioStaticValue *items;
} AudioStaticValue;

static bool audio_mir_static_eval(AudioCompileCtx *audio, Ast *ast,
                                  AudioStaticValue *out) {
  if (!audio || !audio->arena || !ast || !out) {
    return false;
  }

  *out = (AudioStaticValue){
      .kind = AUDIO_STATIC_NONE,
      .ast = ast,
      .type = ast->type,
      .len = 0,
      .items = NULL,
  };

  switch (ast->tag) {
  case AST_INT:
  case AST_UINT64:
    out->kind = AUDIO_STATIC_INT;
    return true;
  case AST_FLOAT:
  case AST_DOUBLE:
    out->kind = AUDIO_STATIC_DOUBLE;
    return true;
  case AST_TUPLE:
  case AST_LIST:
  case AST_ARRAY: {
    size_t len = ast->data.AST_LIST.len;
    AudioStaticValue *items =
        len ? mir_arena_alloc(audio->arena, sizeof(AudioStaticValue) * len,
                              __alignof__(AudioStaticValue))
            : NULL;
    if (len && !items) {
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!audio_mir_static_eval(audio, ast->data.AST_LIST.items + i,
                                 items + i)) {
        return false;
      }
    }
    out->kind = ast->tag == AST_TUPLE   ? AUDIO_STATIC_TUPLE
                : ast->tag == AST_ARRAY ? AUDIO_STATIC_ARRAY_LITERAL
                                        : AUDIO_STATIC_LIST;
    out->len = len;
    out->items = items;
    return true;
  }
  case AST_EMPTY_CONTAINER:
    out->kind = AUDIO_STATIC_LIST;
    return true;
  default:
    return false;
  }
}

static bool audio_mir_static_int_count(AudioStaticValue *value,
                                       size_t *out_count) {
  if (out_count) {
    *out_count = 0;
  }
  if (!value || value->kind != AUDIO_STATIC_INT || !value->ast) {
    return false;
  }

  switch (value->ast->tag) {
  case AST_INT:
    if (value->ast->data.AST_INT.value < 0) {
      return false;
    }
    if (out_count) {
      *out_count = (size_t)value->ast->data.AST_INT.value;
    }
    return true;
  case AST_UINT64:
    if (value->ast->data.AST_UINT64.value > (uint64_t)INT_MAX) {
      return false;
    }
    if (out_count) {
      *out_count = (size_t)value->ast->data.AST_UINT64.value;
    }
    return true;
  default:
    return false;
  }
}

static AudioValue audio_mir_static_index_value(AudioCompileCtx *audio,
                                               Ast *origin, size_t index) {
  if (!audio || !audio->kernel_builder || index > (size_t)INT_MAX) {
    return AUDIO_VALUE_NULL;
  }
  MirValueId value =
      mir_const_int(audio->kernel_builder, &t_int, origin, (int)index);
  return audio_mir_value(&t_int, value, 1);
}

static AudioValue audio_mir_make_builtin_callable(AudioCompileCtx *audio,
                                                  Ast *origin,
                                                  const char *name) {
  if (!audio || !name || audio_mir_builtin_arity(audio, name) <= 0) {
    return AUDIO_VALUE_NULL;
  }

  AudioPartialBuiltin *partial =
      mir_arena_alloc(audio->arena, sizeof(AudioPartialBuiltin),
                      __alignof__(AudioPartialBuiltin));
  if (!partial) {
    return AUDIO_VALUE_NULL;
  }
  *partial = (AudioPartialBuiltin){
      .name = name,
      .type = origin ? origin->type : NULL,
      .argc = 0,
      .args = NULL,
  };
  return (AudioValue){
      .kind = AUDIO_VALUE_PARTIAL_BUILTIN,
      .type = origin ? origin->type : NULL,
      .value = MIR_NO_VALUE,
      .lanes = 0,
      .vec = NULL,
      .synth = NULL,
      .partial_synth = NULL,
      .partial = partial,
  };
}

static AudioValue audio_mir_callable_from_ast(AudioCompileCtx *audio, Ast *ast,
                                              const char *prefix) {
  if (!audio || !ast) {
    return AUDIO_VALUE_NULL;
  }

  if (ast->tag == AST_LAMBDA) {
    static unsigned anon_local_counter = 0;
    const char *local_name =
        mir_arena_printf(audio->arena, "%s.callback.%u",
                         prefix ? prefix : "hof", anon_local_counter++);
    MirAudioSynthSymbol *synth =
        audio_mir_create_local_synth(audio, audio->mir_ctx, local_name, ast);
    return audio_mir_synth_value(ast->type, synth);
  }

  if (ast->tag == AST_IDENTIFIER || ast->tag == AST_RECORD_ACCESS) {
    MirAudioSynthSymbol *synth = audio_mir_ast_audio_symbol(audio, ast);
    if (synth) {
      return audio_mir_synth_value(ast->type, synth);
    }

    const char *name = audio_mir_callable_name(ast);
    AudioValue builtin = audio_mir_make_builtin_callable(audio, ast, name);
    if (audio_mir_value_is_valid(builtin)) {
      return builtin;
    }

    return audio_mir_mir_expr(audio, ast);
  }

  if (ast->tag == AST_APPLICATION) {
    Ast *flat = audio_mir_application_flatten_any(audio, ast);
    MirAudioSynthSymbol *synth =
        audio_mir_application_audio_symbol(audio, flat);
    if (synth) {
      if (audio_mir_application_is_partial(flat)) {
        return audio_mir_make_partial_synth(audio, flat, synth);
      }
      fprintf(stderr, "audio_jit: HOF callback `%s` is already fully applied\n",
              synth->name ? synth->name : "<audio synth>");
      return AUDIO_VALUE_NULL;
    }

    const char *name = audio_mir_application_name(ast);
    if (name && audio_mir_application_is_partial(ast) &&
        audio_mir_builtin_arity(audio, name) > 0) {
      return audio_mir_make_partial_builtin(audio, ast, name);
    }

    AudioValue value = audio_mir_expr(audio, ast);
    if (value.kind == AUDIO_VALUE_PARTIAL_BUILTIN ||
        value.kind == AUDIO_VALUE_PARTIAL_SYNTH ||
        value.kind == AUDIO_VALUE_MIR) {
      return value;
    }
  }

  return AUDIO_VALUE_NULL;
}

static AudioValue audio_mir_static_value_to_audio(AudioCompileCtx *audio,
                                                  AudioStaticValue *value) {
  if (!value || !value->ast) {
    return AUDIO_VALUE_NULL;
  }
  return audio_mir_expr(audio, value->ast);
}

typedef enum AudioHofMode {
  AUDIO_HOF_NONE,
  AUDIO_HOF_MAP,
  AUDIO_HOF_FOLD,
  AUDIO_HOF_FOLDI,
} AudioHofMode;

typedef enum AudioHofDomain {
  AUDIO_HOF_DOMAIN_ANY,
  AUDIO_HOF_DOMAIN_COLLECTION,
  AUDIO_HOF_DOMAIN_LIST,
  AUDIO_HOF_DOMAIN_ARRAY,
  AUDIO_HOF_DOMAIN_INT,
} AudioHofDomain;

typedef struct AudioHofSpec {
  AudioHofMode mode;
  AudioHofDomain domain;
  const char *name;
} AudioHofSpec;

static AudioHofSpec audio_mir_hof_spec(const char *name) {
  AudioHofSpec none = {
      .mode = AUDIO_HOF_NONE,
      .domain = AUDIO_HOF_DOMAIN_ANY,
      .name = name,
  };
  if (!name) {
    return none;
  }

  if (strcmp(name, "map") == 0) {
    return (AudioHofSpec){AUDIO_HOF_MAP, AUDIO_HOF_DOMAIN_COLLECTION, name};
  }
  if (strcmp(name, "fold") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLD, AUDIO_HOF_DOMAIN_ANY, name};
  }
  if (strcmp(name, "foldi") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLDI, AUDIO_HOF_DOMAIN_ANY, name};
  }

  if (strcmp(name, "int_fold") == 0 || strcmp(name, "int_foldi") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLDI, AUDIO_HOF_DOMAIN_INT, name};
  }

  if (strcmp(name, "list_map") == 0) {
    return (AudioHofSpec){AUDIO_HOF_MAP, AUDIO_HOF_DOMAIN_LIST, name};
  }
  if (strcmp(name, "list_fold") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLD, AUDIO_HOF_DOMAIN_LIST, name};
  }
  if (strcmp(name, "list_foldi") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLDI, AUDIO_HOF_DOMAIN_LIST, name};
  }

  if (strcmp(name, "array_map") == 0) {
    return (AudioHofSpec){AUDIO_HOF_MAP, AUDIO_HOF_DOMAIN_ARRAY, name};
  }
  if (strcmp(name, "array_fold") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLD, AUDIO_HOF_DOMAIN_ARRAY, name};
  }
  if (strcmp(name, "array_foldi") == 0) {
    return (AudioHofSpec){AUDIO_HOF_FOLDI, AUDIO_HOF_DOMAIN_ARRAY, name};
  }

  return none;
}

static bool audio_mir_hof_domain_matches(AudioHofSpec spec,
                                         AudioStaticValueKind kind) {
  switch (spec.domain) {
  case AUDIO_HOF_DOMAIN_ANY:
    return kind == AUDIO_STATIC_LIST || kind == AUDIO_STATIC_ARRAY_LITERAL ||
           kind == AUDIO_STATIC_INT;
  case AUDIO_HOF_DOMAIN_COLLECTION:
    return kind == AUDIO_STATIC_LIST || kind == AUDIO_STATIC_ARRAY_LITERAL;
  case AUDIO_HOF_DOMAIN_LIST:
    return kind == AUDIO_STATIC_LIST;
  case AUDIO_HOF_DOMAIN_ARRAY:
    return kind == AUDIO_STATIC_ARRAY_LITERAL;
  case AUDIO_HOF_DOMAIN_INT:
    return kind == AUDIO_STATIC_INT;
  }
  return false;
}

static AudioValue audio_mir_emit_map_result(AudioCompileCtx *audio, Ast *app,
                                            AudioStaticValue *collection,
                                            MirValueIdVec items) {
  if (!audio || !app || !collection) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId result = MIR_NO_VALUE;
  if (collection->kind == AUDIO_STATIC_ARRAY_LITERAL) {
    result = mir_array_literal(audio->kernel_builder, app->type, app, items);
  } else {
    result = mir_list_empty(audio->kernel_builder, app->type, app);
    for (size_t i = items.len; i > 0; i--) {
      result = mir_list_cons(audio->kernel_builder, app->type, app,
                             items.items[i - 1], result);
      if (result == MIR_NO_VALUE) {
        return AUDIO_VALUE_NULL;
      }
    }
  }

  return audio_mir_wrap_mir_value(audio, app, app->type, result);
}

static AudioValue audio_mir_emit_audio_hof(AudioCompileCtx *audio, Ast *app,
                                           const char *name) {
  if (!audio || !app || !name || app->tag != AST_APPLICATION) {
    return AUDIO_VALUE_NULL;
  }

  AudioHofSpec spec = audio_mir_hof_spec(name);
  bool is_map = spec.mode == AUDIO_HOF_MAP;
  bool is_foldi = spec.mode == AUDIO_HOF_FOLDI;
  if (spec.mode == AUDIO_HOF_NONE) {
    return AUDIO_VALUE_NULL;
  }

  size_t expected_argc = is_map ? 2 : 3;
  if (app->data.AST_APPLICATION.len != expected_argc) {
    return AUDIO_VALUE_NULL;
  }

  Ast *callback_ast = app->data.AST_APPLICATION.args;
  Ast *collection_ast = app->data.AST_APPLICATION.args + 1;
  AudioStaticValue collection;
  if (!audio_mir_static_eval(audio, collection_ast, &collection)) {
    fprintf(stderr, "audio_jit: %s domain must be static\n", name);
    return AUDIO_VALUE_NULL;
  }

  bool integer_domain = collection.kind == AUDIO_STATIC_INT;
  if (!audio_mir_hof_domain_matches(spec, collection.kind)) {
    fprintf(stderr, "audio_jit: %s domain has the wrong static shape\n", name);
    return AUDIO_VALUE_NULL;
  }

  size_t integer_count = 0;
  if (integer_domain &&
      !audio_mir_static_int_count(&collection, &integer_count)) {
    fprintf(stderr,
            "audio_jit: %s integer domain must be non-negative and <= %d\n",
            name, INT_MAX);
    return AUDIO_VALUE_NULL;
  }

  AudioValue callback = audio_mir_callable_from_ast(audio, callback_ast, name);
  if (!audio_mir_value_is_valid(callback)) {
    fprintf(stderr, "audio_jit: %s callback is not audio-callable\n", name);
    return AUDIO_VALUE_NULL;
  }

  static unsigned hof_callsite_counter = 0;
  unsigned callsite = hof_callsite_counter++;

  if (is_map) {
    MirValueIdVec results = {0};
    for (size_t i = 0; i < collection.len; i++) {
      AudioValue element =
          audio_mir_static_value_to_audio(audio, collection.items + i);
      if (!audio_mir_value_is_valid(element) ||
          audio_mir_value_lane_count(element) <= 0) {
        return AUDIO_VALUE_NULL;
      }

      const char *prefix =
          mir_arena_printf(audio->arena, "map.%u.%zu", callsite, i);
      AudioValue mapped = audio_mir_apply_audio_value(
          audio, collection.items[i].ast, callback, &element, 1, prefix);
      if (!audio_mir_value_is_valid(mapped) || mapped.value == MIR_NO_VALUE) {
        return AUDIO_VALUE_NULL;
      }
      mir_value_id_vec_push(audio->arena, &results, mapped.value);
    }
    return audio_mir_emit_map_result(audio, app, &collection, results);
  }

  AudioValue acc = audio_mir_expr(audio, app->data.AST_APPLICATION.args + 2);
  if (!audio_mir_value_is_valid(acc) || audio_mir_value_lane_count(acc) <= 0) {
    return AUDIO_VALUE_NULL;
  }

  size_t iteration_count = integer_domain ? integer_count : collection.len;
  for (size_t i = 0; i < iteration_count; i++) {
    Ast *iteration_origin =
        integer_domain ? collection_ast : collection.items[i].ast;
    const char *prefix =
        mir_arena_printf(audio->arena, "%s.%u.%zu", name, callsite, i);
    if (is_foldi) {
      AudioValue index =
          audio_mir_static_index_value(audio, iteration_origin, i);
      if (!audio_mir_value_is_valid(index)) {
        return AUDIO_VALUE_NULL;
      }
      if (integer_domain) {
        AudioValue cb_args[] = {index, acc};
        acc = audio_mir_apply_audio_value(audio, iteration_origin, callback,
                                          cb_args, 2, prefix);
      } else {
        AudioValue element =
            audio_mir_static_value_to_audio(audio, collection.items + i);
        if (!audio_mir_value_is_valid(element) ||
            audio_mir_value_lane_count(element) <= 0) {
          return AUDIO_VALUE_NULL;
        }
        AudioValue cb_args[] = {index, acc, element};
        acc = audio_mir_apply_audio_value(audio, iteration_origin, callback,
                                          cb_args, 3, prefix);
      }
    } else {
      AudioValue element =
          integer_domain
              ? audio_mir_static_index_value(audio, iteration_origin, i)
              : audio_mir_static_value_to_audio(audio, collection.items + i);
      if (!audio_mir_value_is_valid(element) ||
          audio_mir_value_lane_count(element) <= 0) {
        return AUDIO_VALUE_NULL;
      }
      AudioValue cb_args[] = {acc, element};
      acc = audio_mir_apply_audio_value(audio, iteration_origin, callback,
                                        cb_args, 2, prefix);
    }

    if (!audio_mir_value_is_valid(acc) ||
        audio_mir_value_lane_count(acc) <= 0) {
      return AUDIO_VALUE_NULL;
    }
  }

  return acc;
}

static AudioValue audio_mir_let(AudioCompileCtx *audio, Ast *ast) {
  if (!audio || !ast || ast->tag != AST_LET || !ast->data.AST_LET.expr) {
    return AUDIO_VALUE_NULL;
  }

  Ast *binding = ast->data.AST_LET.binding;
  Ast *expr = ast->data.AST_LET.expr;
  Ast *in_expr = ast->data.AST_LET.in_expr;

  if (binding && binding->tag != AST_IDENTIFIER &&
      binding->tag != AST_PLACEHOLDER_ID) {
    return audio_mir_mir_expr(audio, ast);
  }

  if (in_expr && audio->mir_ctx) {
    MirCtx *outer_ctx = audio->mir_ctx;
    AudioLocalBinding *outer_locals = audio->locals;
    MIR_STACK_ALLOC_CTX_PUSH(cont_ctx, audio->kernel_builder, outer_ctx)
    audio->mir_ctx = &cont_ctx;
    if (expr->tag == AST_LAMBDA) {
      if (!audio_mir_bind_local_synth(audio, &cont_ctx, binding, expr)) {
        audio->mir_ctx = outer_ctx;
        audio->locals = outer_locals;
        return AUDIO_VALUE_NULL;
      }
      AudioValue result = audio_mir_expr(audio, in_expr);
      audio->mir_ctx = outer_ctx;
      audio->locals = outer_locals;
      return result;
    }
    AudioValue value = audio_mir_expr(audio, expr);
    if (!audio_mir_bind_local_value(audio, binding, value)) {
      audio->mir_ctx = outer_ctx;
      audio->locals = outer_locals;
      return AUDIO_VALUE_NULL;
    }
    AudioValue result = audio_mir_expr(audio, in_expr);
    audio->mir_ctx = outer_ctx;
    audio->locals = outer_locals;
    return result;
  }

  if (expr->tag == AST_LAMBDA) {
    if (!audio_mir_bind_local_synth(audio, audio->mir_ctx, binding, expr)) {
      return AUDIO_VALUE_NULL;
    }
    MirValueId unit = mir_const_void(audio->kernel_builder, &t_void, ast);
    return audio_mir_value(&t_void, unit, 1);
  }

  AudioValue value = audio_mir_expr(audio, expr);
  if (!audio_mir_bind_local_value(audio, binding, value)) {
    return AUDIO_VALUE_NULL;
  }
  return value;
}

static AudioValue audio_mir_expr(AudioCompileCtx *audio, Ast *ast) {
  if (!audio || !ast) {
    return AUDIO_VALUE_NULL;
  }

  switch (ast->tag) {
  case AST_BODY: {
    AudioValue value = AUDIO_VALUE_NULL;
    for (AstList *l = ast->data.AST_BODY.stmts; l; l = l->next) {
      value = audio_mir_expr(audio, l->ast);
    }
    return value;
  }
  case AST_LET:
    return audio_mir_let(audio, ast);
  case AST_ARRAY:
    return audio_mir_array_literal(audio, ast);
  case AST_IDENTIFIER: {
    AudioValue *local =
        audio_mir_lookup_local(audio, ast->data.AST_IDENTIFIER.value);
    if (local) {
      return *local;
    }
    MirAudioSynthSymbol *synth = audio_mir_ast_audio_symbol(audio, ast);
    return synth ? audio_mir_synth_value(ast->type, synth)
                 : audio_mir_mir_expr(audio, ast);
  }
  case AST_APPLICATION: {
    ast = audio_mir_application_flatten_if_saturated(audio, ast);
    const char *name = audio_mir_application_name(ast);
    const char *callable_name = audio_mir_application_callable_name(ast);

    if (audio_mir_hof_spec(callable_name).mode != AUDIO_HOF_NONE) {
      AudioValue hof = audio_mir_emit_audio_hof(audio, ast, callable_name);
      if (audio_mir_value_is_valid(hof)) {
        return hof;
      }
      return AUDIO_VALUE_NULL;
    }

    if (name && audio_mir_application_is_partial(ast) &&
        audio_mir_builtin_arity(audio, name) > 0) {
      return audio_mir_make_partial_builtin(audio, ast, name);
    }

    AudioValue *local = name ? audio_mir_lookup_local(audio, name) : NULL;
    if (local && local->kind == AUDIO_VALUE_PARTIAL_BUILTIN) {
      return audio_mir_apply_partial_builtin(audio, ast, local->partial);
    }

    Ast *synth_app = audio_mir_application_flatten_any(audio, ast);
    MirAudioSynthSymbol *synth =
        audio_mir_application_audio_symbol(audio, synth_app);
    if (synth) {
      if (audio_mir_application_is_partial(synth_app)) {
        return audio_mir_make_partial_synth(audio, synth_app, synth);
      }
      return audio_mir_emit_synth_in_audio_context(audio, synth_app, synth);
    }

    if (audio_mir_application_is_partial(ast)) {
      return audio_mir_mir_expr(audio, ast);
    }

    if (!name && ast->data.AST_APPLICATION.function &&
        ast->data.AST_APPLICATION.function->tag == AST_APPLICATION) {
      AudioValue callee =
          audio_mir_expr(audio, ast->data.AST_APPLICATION.function);
      if (callee.kind == AUDIO_VALUE_PARTIAL_BUILTIN) {
        return audio_mir_apply_partial_builtin(audio, ast, callee.partial);
      }
      if (callee.kind == AUDIO_VALUE_PARTIAL_SYNTH) {
        size_t argc = audio_mir_application_value_arg_count(ast);
        AudioValue *args =
            argc ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                                   __alignof__(AudioValue))
                 : NULL;
        if (argc && !args) {
          return AUDIO_VALUE_NULL;
        }
        for (size_t i = 0; i < argc; i++) {
          args[i] = audio_mir_expr(audio, ast->data.AST_APPLICATION.args + i);
          if (!audio_mir_value_is_valid(args[i])) {
            return AUDIO_VALUE_NULL;
          }
        }
        return audio_mir_apply_audio_value(audio, ast, callee, args, argc,
                                           NULL);
      }
    }

    if (name) {

      if (strcmp(name, "array_size") == 0) {
        return audio_mir_array_size_value(audio, ast);
      }

      if (strcmp(name, "array_at") == 0) {
        return audio_mir_array_at_value(audio, ast);
      }

      if (strcmp(name, "array_set") == 0) {
        return audio_mir_array_set_value(audio, ast);
      }

      if (strcmp(name, "+") == 0) {
        return sum_signals(audio, ast);
      }

      if (strcmp(name, "-") == 0) {
        return sub_signals(audio, ast);
      }

      if (strcmp(name, "*") == 0) {
        return mul_signals(audio, ast);
      }

      if (strcmp(name, "/") == 0) {
        return div_signals(audio, ast);
      }

      if (strcmp(name, "%") == 0) {
        return mod_signals(audio, ast);
      }

      if (strcmp(name, ">=") == 0) {
        return gte_signals(audio, ast);
      }

      if (strcmp(name, ">") == 0) {
        return gt_signals(audio, ast);
      }

      if (strcmp(name, "<=") == 0) {
        return lte_signals(audio, ast);
      }

      if (strcmp(name, "<") == 0) {
        return lt_signals(audio, ast);
      }

      if (strcmp(name, "~") == 0) {
        return multichannel_operator(audio, ast);
      }

      if (audio_mir_builtin_arity(audio, name) > 0) {
        size_t argc = audio_mir_application_value_arg_count(ast);
        AudioValue *args =
            argc ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                                   __alignof__(AudioValue))
                 : NULL;
        if (argc && !args) {
          return AUDIO_VALUE_NULL;
        }
        for (size_t i = 0; i < argc; i++) {
          args[i] = audio_mir_expr(audio, ast->data.AST_APPLICATION.args + i);
          if (!audio_mir_value_is_valid(args[i])) {
            return AUDIO_VALUE_NULL;
          }
        }
        return audio_mir_emit_builtin_values(audio, ast, name, args, argc);
      }
    }

    return audio_mir_call_application(audio, ast);
  }
  default:
    return audio_mir_mir_expr(audio, ast);
  }
}

static bool audio_mir_bind_kernel_abi(AudioCompileCtx *audio, MirCtx *mir_ctx) {
  if (!audio || !audio->kernel_fn || !mir_ctx ||
      audio->kernel_fn->params.len < 4) {
    return false;
  }

  for (size_t i = 0; i < audio->kernel_fn->params.len; i++) {
    MirParam *param = &audio->kernel_fn->params.items[i];
    if (!audio_mir_bind_value_name(mir_ctx, param->name, param->value)) {
      return false;
    }
  }

  audio->node_param = audio->kernel_fn->params.items[0].value;
  audio->state_param = audio->kernel_fn->params.items[1].value;
  audio->frame_param = audio->kernel_fn->params.items[2].value;
  audio->spf_param = audio->kernel_fn->params.items[3].value;
  return true;
}

static void audio_mir_emit_kernel(AudioCompileCtx *audio) {
  if (!audio || !audio->bundle || !audio->kernel_fn || !audio->lambda ||
      audio->lambda->tag != AST_LAMBDA) {
    return;
  }

  MirCtx kernel_ctx = {
      .env = audio->parent_ctx && audio->parent_ctx->env
                 ? audio->parent_ctx->env
                 : audio->program->type_env,
      .frame = NULL,
      .current_module = audio->parent_ctx ? audio->parent_ctx->current_module
                                          : audio->program->root_module,
      .export_bindings = false,
      .prefer_global_loads = true,
      .extension_kind = MIR_AUDIO_CONTEXT_KIND,
      .extension_data = audio,
  };
  ht frame_table;
  MirStackFrame frame;
  MirStackFrame *parent = audio->parent_ctx ? audio->parent_ctx->frame : NULL;
  mir_stack_frame_init(audio->kernel_fn->arena, &frame_table, &frame, parent);
  kernel_ctx.frame = &frame;
  audio->mir_ctx = &kernel_ctx;

  MirValueId kernel_value = MIR_NO_VALUE;
  if (audio_mir_bind_kernel_abi(audio, &kernel_ctx) &&
      audio_mir_bind_kernel_lambda_params(audio->bundle, &kernel_ctx)) {
    AudioValue value =
        audio_mir_expr(audio, audio->lambda->data.AST_LAMBDA.body);
    kernel_value = value.value;
    if (value.type) {
      audio_mir_set_fn_return_type(audio->arena, audio->kernel_fn, value.type);
    }
  }

  if (kernel_value == MIR_NO_VALUE) {
    kernel_value = mir_const_undef(audio->kernel_builder,
                                   audio_mir_kernel_return_type(audio->bundle),
                                   audio->app);
  }
  mir_builder_set_return(audio->kernel_builder, kernel_value);
  audio->mir_ctx = NULL;
}

static void audio_mir_emit_init(AudioCompileCtx *audio) {
  if (!audio || !audio->init_fn || audio->init_fn->params.len < 1) {
    return;
  }

  MirValueId state = audio->init_fn->params.items[0].value;
  for (AudioStateSlot *slot = audio->state_slots; slot; slot = slot->next) {
    if (slot->zero_bytes) {
      MirValueId ptr =
          audio_mir_state_slot_ptr(audio, audio->init_builder, audio->app,
                                   state, slot->offset, &t_char);
      audio_mir_emit_memzero(audio, audio->app, ptr, slot->size);
      continue;
    }

    MirValueId ptr =
        audio_mir_state_slot_ptr(audio, audio->init_builder, audio->app, state,
                                 slot->offset, slot->type);
    MirValueId zero =
        audio_mir_zero_value(audio->init_builder, slot->type, audio->app);
    if (ptr != MIR_NO_VALUE && zero != MIR_NO_VALUE) {
      mir_ptr_store(audio->init_builder, audio->app, ptr, zero);
    }
  }

  audio_mir_emit_state_init_stores(audio, state);
  audio_mir_emit_init_calls(audio, audio->init_calls, state);

  MirValueId init_ret =
      mir_const_void(audio->init_builder, &t_void, audio->app);
  mir_builder_set_return(audio->init_builder, init_ret);
}

static MirValueId audio_mir_read_frame_input(AudioCompileCtx *audio,
                                             size_t input_index) {
  MirBuilder *b = audio->frame_builder;
  MirValueId index = mir_const_int(b, &t_int, audio->app, (int)input_index);
  MirValueId slot = mir_ptr_offset(b, audio->ptr_ptr_type, audio->app,
                                   audio->inputs_param, index);
  MirValueId inlet = mir_ptr_load(b, &t_ptr, audio->app, slot);

  Type *read_params[] = {&t_ptr, &t_int};
  Type *read_type =
      audio_mir_fn_type(audio->arena, read_params,
                        sizeof(read_params) / sizeof(read_params[0]), &t_num);
  MirValueId read_fn =
      audio_mir_extern_ref(b, "ylc_read_inlet_node_i32", read_type, audio->app);
  MirValueId read_args[] = {inlet, audio->frame_param};
  return mir_call_value(b, &t_num, audio->app, read_fn, read_type, read_args,
                        sizeof(read_args) / sizeof(read_args[0]));
}

static void audio_mir_emit_frame_adapter(AudioCompileCtx *audio) {
  MirBuilder *b = audio->frame_builder;
  audio->node_param = audio->frame_fn->params.items[0].value;
  audio->state_param = audio->frame_fn->params.items[1].value;
  audio->inputs_param = audio->frame_fn->params.items[2].value;
  audio->frame_param = audio->frame_fn->params.items[3].value;
  audio->spf_param = audio->frame_fn->params.items[4].value;

  size_t input_count = audio->bundle ? (size_t)audio->bundle->num_inputs : 0;
  size_t kernel_argc = input_count + 4;
  MirValueId *kernel_args = mir_arena_alloc(
      audio->arena, sizeof(MirValueId) * kernel_argc, __alignof__(MirValueId));

  kernel_args[0] = audio->node_param;
  kernel_args[1] = audio->state_param;
  kernel_args[2] = audio->frame_param;
  kernel_args[3] = audio->spf_param;
  for (size_t i = 0; i < input_count; i++) {
    MirValueId sample = audio_mir_read_frame_input(audio, i);
    Type *formal = i + 4 < audio->kernel_fn->params.len
                       ? audio->kernel_fn->params.items[i + 4].type
                       : &t_num;
    if (sample != MIR_NO_VALUE && formal && formal->kind != T_NUM) {
      sample = mir_primitive_cast(b, &t_num, formal, audio->app, sample);
    }
    kernel_args[i + 4] = sample;
  }

  Type *kernel_result_type = fn_return_type(audio->kernel_fn->type);
  MirValueId kernel_ref =
      mir_fn_ref(b, audio->kernel_fn->type, audio->app, audio->kernel_fn);
  MirValueId sample =
      mir_call_value(b, kernel_result_type, audio->app, kernel_ref,
                     audio->kernel_fn->type, kernel_args, kernel_argc);

  if (sample != MIR_NO_VALUE && kernel_result_type &&
      kernel_result_type->kind != T_VOID) {
    Type *output_params[] = {&t_ptr};
    Type *output_type =
        audio_mir_fn_type(audio->arena, output_params,
                          sizeof(output_params) / sizeof(output_params[0]),
                          audio->ptr_double_type);
    MirValueId output_fn =
        audio_mir_extern_ref(b, "ylc_get_output_buf", output_type, audio->app);
    MirValueId output_buf =
        mir_call_value(b, audio->ptr_double_type, audio->app, output_fn,
                       output_type, (MirValueId[]){audio->node_param}, 1);

    if (audio_mir_is_tuple_type(kernel_result_type) &&
        kernel_result_type->data.T_CONS.args &&
        kernel_result_type->data.T_CONS.num_args > 0) {
      int lanes = kernel_result_type->data.T_CONS.num_args;
      MirValueId lane_count = mir_const_int(b, &t_int, audio->app, lanes);
      MirValueId frame_base =
          mir_imul(b, &t_int, audio->app, audio->frame_param, lane_count);
      for (int lane = 0; lane < lanes; lane++) {
        Type *lane_type = kernel_result_type->data.T_CONS.args[lane]
                              ? kernel_result_type->data.T_CONS.args[lane]
                              : &t_num;
        MirValueId lane_sample =
            mir_tuple_get(b, lane_type, audio->app, sample, (size_t)lane);
        if (lane_type->kind != T_NUM) {
          lane_sample =
              mir_primitive_cast(b, lane_type, &t_num, audio->app, lane_sample);
        }
        MirValueId lane_offset = frame_base;
        if (lane > 0) {
          lane_offset = mir_iadd(b, &t_int, audio->app, frame_base,
                                 mir_const_int(b, &t_int, audio->app, lane));
        }
        MirValueId out_ptr = mir_ptr_offset(
            b, audio->ptr_double_type, audio->app, output_buf, lane_offset);
        mir_ptr_store(b, audio->app, out_ptr, lane_sample);
      }
    } else {
      if (kernel_result_type->kind != T_NUM) {
        sample = mir_primitive_cast(b, kernel_result_type, &t_num, audio->app,
                                    sample);
      }
      MirValueId out_ptr = mir_ptr_offset(b, audio->ptr_double_type, audio->app,
                                          output_buf, audio->frame_param);
      if (out_ptr != MIR_NO_VALUE && sample != MIR_NO_VALUE) {
        mir_ptr_store(b, audio->app, out_ptr, sample);
      }
    }
  }

  mir_builder_set_return(b, mir_const_void(b, &t_void, audio->app));
}

static void audio_mir_emit_constructor(AudioCompileCtx *audio) {
  MirBuilder *b = audio->cons_builder;
  Type *create_params[] = {audio->frame_fn->type, &t_int, &t_int, &t_int,
                           &t_ptr};
  Type *create_type = audio_mir_fn_type(
      audio->arena, create_params,
      sizeof(create_params) / sizeof(create_params[0]), &t_ptr);
  MirValueId create_fn = audio_mir_extern_ref(b, "ylc_create_audio_frame_node",
                                              create_type, audio->app);
  MirValueId frame_ref =
      mir_fn_ref(b, audio->frame_fn->type, audio->app, audio->frame_fn);
  const char *meta =
      audio->bundle && audio->bundle->name ? audio->bundle->name : "audio";
  MirValueId meta_str =
      mir_const_string(b, &t_ptr, audio->app, meta, strlen(meta));
  MirValueId create_args[] = {
      frame_ref,
      mir_const_int(b, &t_int, audio->app,
                    audio->bundle ? audio->bundle->num_inputs : 0),
      mir_const_int(b, &t_int, audio->app,
                    audio_mir_kernel_output_lanes(audio->kernel_fn)),
      mir_const_int(b, &t_int, audio->app,
                    audio->bundle ? audio->bundle->state_bytes : 0),
      meta_str,
  };
  MirValueId node =
      mir_call_value(b, &t_ptr, audio->app, create_fn, create_type, create_args,
                     sizeof(create_args) / sizeof(create_args[0]));

  Type *state_params[] = {&t_ptr};
  Type *state_type = audio_mir_fn_type(
      audio->arena, state_params,
      sizeof(state_params) / sizeof(state_params[0]), audio->ptr_char_type);
  MirValueId state_fn = audio_mir_extern_ref(b, "ylc_audio_node_inline_state",
                                             state_type, audio->app);
  MirValueId state =
      mir_call_value(b, audio->ptr_char_type, audio->app, state_fn, state_type,
                     (MirValueId[]){node}, 1);
  MirValueId init_ref =
      mir_fn_ref(b, audio->init_fn->type, audio->app, audio->init_fn);
  mir_call_value(b, &t_void, audio->app, init_ref, audio->init_fn->type,
                 (MirValueId[]){state}, 1);

  size_t input_count = audio->bundle ? (size_t)audio->bundle->num_inputs : 0;
  if (input_count > 0) {
    Type *plug_params[] = {&t_int, &t_ptr, &t_ptr};
    Type *plug_type = audio_mir_fn_type(
        audio->arena, plug_params, sizeof(plug_params) / sizeof(plug_params[0]),
        &t_void);
    MirValueId plug_ref =
        audio_mir_extern_ref(b, "node_connect_input", plug_type, audio->app);
    for (size_t i = 0; i < input_count && i < audio->cons_fn->params.len; i++) {
      MirParam *param = &audio->cons_fn->params.items[i];
      MirValueId input_node =
          audio_mir_value_as_node(b, audio->app, param->value, param->type);

      MirValueId plug_args[] = {
          mir_const_int(b, &t_int, audio->app, (int)i),
          node,
          input_node,
      };
      mir_call_value(b, &t_void, audio->app, plug_ref, plug_type, plug_args,
                     sizeof(plug_args) / sizeof(plug_args[0]));
    }
  }

  mir_builder_set_return(b, node);
}

static void audio_mir_emit_local_constructor(AudioCompileCtx *audio) {
  if (!audio || !audio->cons_builder || !audio->cons_fn ||
      audio->cons_fn->params.len < 2) {
    return;
  }

  MirBuilder *b = audio->cons_builder;
  MirValueId parent_state = audio->cons_fn->params.items[0].value;
  MirValueId state_offset = audio->cons_fn->params.items[1].value;
  MirValueId state = mir_ptr_offset(b, audio->ptr_char_type, audio->app,
                                    parent_state, state_offset);
  Type *ret_type = fn_return_type(audio->cons_fn->type);
  if (state != MIR_NO_VALUE && ret_type &&
      !types_equal(ret_type, audio->ptr_char_type)) {
    state = mir_primitive_cast(b, audio->ptr_char_type, ret_type, audio->app,
                               state);
  }
  if (state == MIR_NO_VALUE) {
    state = mir_const_undef(b, ret_type ? ret_type : audio->ptr_char_type,
                            audio->app);
  }
  mir_builder_set_return(b, state);
}

static bool audio_mir_bundle_opt_ctx_init(AudioBundleOptCtx *opt,
                                          AudioCompileCtx *audio) {
  if (!opt || !audio || !audio->program || !audio->bundle_fns_len) {
    return false;
  }

  *opt = (AudioBundleOptCtx){
      .program = audio->program,
      .arena = audio->arena,
      .cons_fn = audio->cons_fn,
      .init_fn = audio->init_fn,
      .kernel_fn = audio->kernel_fn,
      .frame_fn = audio->frame_fn,
      .fns = audio->bundle_fns,
      .fns_len = audio->bundle_fns_len,
  };
  return opt->cons_fn && opt->init_fn && opt->kernel_fn;
}

static void audio_mir_bundle_constant_fold(AudioBundleOptCtx *opt) {
  if (!opt) {
    return;
  }

  // Stub pass entry: this sees cons/init/kernel/frame together, so constants
  // captured in cons can later be propagated into init/kernel/frame before
  // LLVM.
  for (size_t i = 0; i < opt->fns_len; i++) {
    (void)opt->fns[i];
  }
}

static void audio_mir_bundle_inline_kernels(AudioBundleOptCtx *opt) {
  if (!opt) {
    return;
  }

  // Stub pass entry: nested synth kernel calls are emitted as direct MIR calls,
  // so this can inline them before whole-program MIR/LLVM lowering.
  (void)opt->kernel_fn;
  (void)opt->frame_fn;
}

static void audio_mir_optimize_bundle(AudioCompileCtx *audio) {
  AudioBundleOptCtx opt;
  if (!audio_mir_bundle_opt_ctx_init(&opt, audio)) {
    return;
  }

  audio_mir_bundle_constant_fold(&opt);
  audio_mir_bundle_inline_kernels(&opt);
}

static void audio_mir_emit_synth_stubs(MirAudioSynthBuildCtx *ctx) {
  AudioCompileCtx audio;
  audio_mir_compile_ctx_init(&audio, ctx);

  audio_mir_emit_kernel(&audio);
  audio_mir_emit_init(&audio);
  audio_mir_emit_frame_adapter(&audio);
  audio_mir_emit_constructor(&audio);
  audio_mir_optimize_bundle(&audio);
}

static void audio_mir_emit_local_synth_stubs(MirAudioSynthBuildCtx *ctx) {
  AudioCompileCtx audio;
  audio_mir_compile_ctx_init(&audio, ctx);

  audio_mir_emit_kernel(&audio);
  audio_mir_emit_init(&audio);
  audio_mir_emit_local_constructor(&audio);
  audio_mir_optimize_bundle(&audio);
}

static bool audio_mir_in_audio_context(MirCtx *ctx) {
  return ctx && ctx->extension_kind &&
         strcmp(ctx->extension_kind, MIR_AUDIO_CONTEXT_KIND) == 0;
}

static MirValueId MirCompileAudioNodeBuiltinHandler(MirBuilder *builder,
                                                    Ast *app, MirCtx *ctx,
                                                    MirBuiltinSymbol *symbol) {
  if (!symbol || symbol->kind != MIR_BUILTIN_SYMBOL_EXTENSION ||
      symbol->handler != MirCompileAudioNodeBuiltinHandler) {
    return MIR_NO_VALUE;
  }
  const char *node_ctor = audio_mir_osc_node_ctor_name(symbol->name);
  if (!node_ctor) {
    return MIR_NO_VALUE;
  }

  Ast *freq_ast = app->data.AST_APPLICATION.args;
  MirValueId freq = mir_expr(builder, freq_ast, ctx);
  Type *freq_type = freq_ast && freq_ast->type ? freq_ast->type : &t_num;
  MirValueId input = audio_mir_value_as_node(builder, app, freq, freq_type);
  app->type = &t_ptr;
  return audio_mir_call_unary_node(builder, app, node_ctor, input);
}

static MirValueId
audio_mir_call_synth_outside_context(MirBuilder *builder, Ast *app, MirCtx *ctx,
                                     MirAudioSynthSymbol *synth) {
  if (!builder || !builder->program || !builder->fn || !app || !synth ||
      !synth->ctor_fn) {
    return MIR_NO_VALUE;
  }
  if (synth->scope == AUDIO_SYNTH_LOCAL) {
    fprintf(stderr,
            "audio_jit: local audio function `%s` cannot escape its enclosing "
            "@Audio function\n",
            synth->name ? synth->name : "<anonymous>");
    return MIR_NO_VALUE;
  }

  size_t argc = audio_mir_application_value_arg_count(app);
  MirValueId *args =
      argc ? mir_arena_alloc(builder->fn->arena, sizeof(MirValueId) * argc,
                             __alignof__(MirValueId))
           : NULL;
  if (argc && !args) {
    return MIR_NO_VALUE;
  }

  bool has_node_arg = false;
  for (size_t i = 0; i < argc; i++) {
    Ast *arg_ast = app->data.AST_APPLICATION.args + i;
    args[i] = mir_expr(builder, arg_ast, ctx);
    if (args[i] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
    Type *arg_type = mir_function_value_type(builder->fn, args[i]);
    if (!arg_type) {
      arg_type = arg_ast ? arg_ast->type : NULL;
    }
    if (audio_mir_is_ptr_type(arg_type)) {
      has_node_arg = true;
    }
  }

  app->type = &t_ptr;

  if (has_node_arg) {
    Type *create_params[] = {synth->frame_fn->type, &t_int, &t_int, &t_int,
                             &t_ptr};
    Type *create_type = audio_mir_fn_type(
        builder->fn->arena, create_params,
        sizeof(create_params) / sizeof(create_params[0]), &t_ptr);
    MirValueId create_fn = audio_mir_extern_ref(
        builder, "ylc_create_audio_frame_node", create_type, app);
    MirValueId frame_ref =
        mir_fn_ref(builder, synth->frame_fn->type, app, synth->frame_fn);
    const char *meta_name = synth->name ? synth->name : "audio";
    MirValueId meta =
        mir_const_string(builder, &t_ptr, app, meta_name, strlen(meta_name));
    MirValueId input_count_value =
        mir_const_int(builder, &t_int, app, synth->num_inputs);
    MirValueId output_lanes = mir_const_int(
        builder, &t_int, app, audio_mir_kernel_output_lanes(synth->kernel_fn));
    MirValueId state_bytes =
        mir_const_int(builder, &t_int, app, synth->state_bytes);
    MirValueId create_args[] = {frame_ref, input_count_value, output_lanes,
                                state_bytes, meta};
    MirValueId node = mir_call_value(
        builder, &t_ptr, app, create_fn, create_type, create_args,
        sizeof(create_args) / sizeof(create_args[0]));
    if (node == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }

    Type *state_params[] = {&t_ptr};
    Type *state_type =
        audio_mir_fn_type(builder->fn->arena, state_params,
                          sizeof(state_params) / sizeof(state_params[0]),
                          audio_mir_ptr_to(builder->fn->arena, &t_char));
    MirValueId state_fn = audio_mir_extern_ref(
        builder, "ylc_audio_node_inline_state", state_type, app);
    MirValueId state_args[] = {node};
    MirValueId state = mir_call_value(builder, fn_return_type(state_type), app,
                                      state_fn, state_type, state_args, 1);
    MirValueId init_ref =
        mir_fn_ref(builder, synth->init_fn->type, app, synth->init_fn);
    MirValueId init_args[] = {state};
    mir_call_value(builder, &t_void, app, init_ref, synth->init_fn->type,
                   init_args, 1);

    Type *plug_params[] = {&t_int, &t_ptr, &t_ptr};
    Type *plug_type = audio_mir_fn_type(
        builder->fn->arena, plug_params,
        sizeof(plug_params) / sizeof(plug_params[0]), &t_void);
    MirValueId plug =
        audio_mir_extern_ref(builder, "node_connect_input", plug_type, app);
    size_t input_count = synth->num_inputs > 0 ? (size_t)synth->num_inputs : 0;
    for (size_t i = 0; i < argc && i < input_count; i++) {
      Ast *arg_ast = app->data.AST_APPLICATION.args + i;
      Type *arg_type = mir_function_value_type(builder->fn, args[i]);
      if (!arg_type) {
        arg_type = arg_ast ? arg_ast->type : NULL;
      }
      MirValueId input = audio_mir_value_as_node(builder, app, args[i],
                                                 arg_type ? arg_type : &t_num);
      MirValueId input_index = mir_const_int(builder, &t_int, app, (int)i);
      MirValueId plug_args[] = {input_index, node, input};
      mir_call_value(builder, &t_void, app, plug, plug_type, plug_args,
                     sizeof(plug_args) / sizeof(plug_args[0]));
    }
    return node;
  }

  MirValueId ctor =
      mir_fn_ref(builder, synth->ctor_fn->type, app, synth->ctor_fn);
  return mir_call_value(builder, &t_ptr, app, ctor, synth->ctor_fn->type, args,
                        argc);
}

static MirValueId audio_mir_nested_synth_state(AudioCompileCtx *audio,
                                               MirBuilder *builder, Ast *origin,
                                               MirAudioSynthSymbol *synth,
                                               Type *state_type, int lane,
                                               int lanes,
                                               const char *instance_prefix) {
  if (!audio || !builder || !synth || audio->state_param == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  size_t state_bytes = synth->state_bytes > 0 ? (size_t)synth->state_bytes : 1;
  const char *slot_base = instance_prefix
                              ? instance_prefix
                              : (synth->name ? synth->name : "sub_synth");
  const char *slot_name =
      lanes > 1 ? mir_arena_printf(audio->arena, "%s.%d", slot_base, lane)
                : slot_base;
  AudioStateSlot *slot =
      audio_mir_reserve_state_slot(audio, &t_char, state_bytes, 8, slot_name);
  if (!slot) {
    return MIR_NO_VALUE;
  }
  Type *init_state_type = synth->init_fn && synth->init_fn->params.len > 0
                              ? synth->init_fn->params.items[0].type
                              : audio->ptr_char_type;
  audio_mir_add_init_call(audio, synth->init_fn, slot->offset, init_state_type,
                          slot_name);

  MirValueId state = audio_mir_state_slot_ptr(
      audio, builder, origin, audio->state_param, slot->offset, &t_char);
  if (state != MIR_NO_VALUE && state_type &&
      state_type != audio->ptr_char_type) {
    state = mir_primitive_cast(builder, audio->ptr_char_type, state_type,
                               origin, state);
  }
  return state;
}

static AudioValue
audio_mir_apply_audio_synth(AudioCompileCtx *audio, Ast *origin,
                            MirAudioSynthSymbol *synth, AudioValue *bound_args,
                            size_t bound_arg_count, AudioValue *args,
                            size_t arg_count, const char *instance_prefix) {
  if (!audio || !audio->kernel_builder || !audio->mir_ctx || !origin ||
      !synth || !synth->kernel_fn) {
    return AUDIO_VALUE_NULL;
  }

  MirBuilder *builder = audio->kernel_builder;
  MirCtx *ctx = audio->mir_ctx;

  size_t explicit_arg_count = bound_arg_count + arg_count;
  if (explicit_arg_count != (size_t)synth->num_inputs) {
    fprintf(stderr, "audio_jit: audio synth `%s` expected %d args, got %zu\n",
            synth->name ? synth->name : "<anonymous>", synth->num_inputs,
            explicit_arg_count);
    return AUDIO_VALUE_NULL;
  }

  size_t capture_count = synth->capture_count;
  size_t call_value_count = capture_count + explicit_arg_count;
  AudioValue *call_values =
      call_value_count
          ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * call_value_count,
                            __alignof__(AudioValue))
          : NULL;
  if (call_value_count && !call_values) {
    return AUDIO_VALUE_NULL;
  }

  for (size_t i = 0; i < capture_count; i++) {
    Ast *capture = synth->capture_asts && synth->capture_asts[i]
                       ? synth->capture_asts[i]
                       : NULL;
    if (!capture) {
      return AUDIO_VALUE_NULL;
    }
    call_values[i] = audio_mir_expr(audio, capture);
    if (!audio_mir_value_is_valid(call_values[i]) ||
        audio_mir_value_lane_count(call_values[i]) <= 0) {
      fprintf(stderr,
              "audio_jit: failed to lower capture for local audio synth `%s`\n",
              synth->name ? synth->name : "<anonymous>");
      return AUDIO_VALUE_NULL;
    }
  }

  for (size_t i = 0; i < bound_arg_count; i++) {
    call_values[capture_count + i] = bound_args[i];
  }
  for (size_t i = 0; i < arg_count; i++) {
    call_values[capture_count + bound_arg_count + i] = args[i];
  }

  int lanes =
      call_value_count ? audio_mir_max_lanes(call_values, call_value_count) : 1;
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId node =
      audio->node_param != MIR_NO_VALUE ? audio->node_param : MIR_NO_VALUE;
  if (node == MIR_NO_VALUE && !mir_ctx_lookup_value(ctx, "node", &node)) {
    node = mir_const_undef(builder, &t_ptr, origin);
  }
  if (node == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  Type *state_type =
      synth->kernel_fn->params.len > 1 && synth->kernel_fn->params.items[1].type
          ? synth->kernel_fn->params.items[1].type
          : audio->ptr_char_type;
  Type *result_type = fn_return_type(synth->kernel_fn->type);
  if (!result_type) {
    result_type = origin->type ? origin->type : &t_num;
  }

  size_t total_argc = call_value_count + 4;
  MirValueId kernel = audio_mir_fn_ref(audio, builder, synth->kernel_fn->type,
                                       origin, synth->kernel_fn);
  MirValueId *samples =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !samples) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId state =
        audio_mir_nested_synth_state(audio, builder, origin, synth, state_type,
                                     lane, lanes, instance_prefix);
    if (state == MIR_NO_VALUE) {
      state = mir_const_undef(builder, state_type, origin);
    }

    MirValueId *args = mir_arena_alloc(
        audio->arena, sizeof(MirValueId) * total_argc, __alignof__(MirValueId));
    if (!args || state == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }

    args[0] = node;
    args[1] = state;
    args[2] = audio->frame_param != MIR_NO_VALUE
                  ? audio->frame_param
                  : mir_const_undef(builder, &t_int, origin);
    args[3] = audio->spf_param != MIR_NO_VALUE
                  ? audio->spf_param
                  : mir_const_undef(builder, &t_num, origin);

    for (size_t i = 0; i < call_value_count; i++) {
      Type *formal = i + 4 < synth->kernel_fn->params.len
                         ? synth->kernel_fn->params.items[i + 4].type
                         : audio_mir_value_lane_type(call_values[i], lane);
      args[i + 4] =
          audio_mir_lane_as_formal(audio, origin, call_values[i], lane, formal);
      if (args[i + 4] == MIR_NO_VALUE) {
        return AUDIO_VALUE_NULL;
      }
    }

    MirValueId sample =
        mir_call_value(builder, result_type, origin, kernel,
                       synth->kernel_fn->type, args, total_argc);
    if (sample == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      if (origin->tag == AST_APPLICATION) {
        origin->type = result_type;
      }
      return audio_mir_wrap_mir_value(audio, origin, result_type, sample);
    }
    samples[lane] = sample;
  }

  Type *preferred_type =
      origin->tag == AST_APPLICATION ? origin->type : result_type;
  return audio_mir_multi_value_typed(audio, origin, preferred_type, result_type,
                                     samples, lanes);
}

static AudioValue audio_mir_apply_mir_callable_value(AudioCompileCtx *audio,
                                                     Ast *origin,
                                                     AudioValue callable,
                                                     AudioValue *args,
                                                     size_t arg_count) {
  if (!audio || !origin || callable.value == MIR_NO_VALUE || !callable.type ||
      callable.type->kind != T_FN || (arg_count && !args)) {
    return AUDIO_VALUE_NULL;
  }

  int lanes = arg_count ? audio_mir_max_lanes(args, arg_count) : 1;
  if (lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }

  Type *result_type = callable.type;
  for (size_t i = 0; i < arg_count && result_type && result_type->kind == T_FN;
       i++) {
    result_type = result_type->data.T_FN.to;
  }
  if (!result_type || result_type->kind == T_FN) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId *results =
      lanes > 1 ? audio_mir_alloc_lane_values(audio, lanes) : NULL;
  if (lanes > 1 && !results) {
    return AUDIO_VALUE_NULL;
  }

  for (int lane = 0; lane < lanes; lane++) {
    MirValueId *call_args =
        arg_count
            ? mir_arena_alloc(audio->arena, sizeof(MirValueId) * arg_count,
                              __alignof__(MirValueId))
            : NULL;
    if (arg_count && !call_args) {
      return AUDIO_VALUE_NULL;
    }

    for (size_t i = 0; i < arg_count; i++) {
      call_args[i] =
          audio_mir_lane_as_formal(audio, origin, args[i], lane,
                                   audio_mir_fn_arg_type(callable.type, i));
      if (call_args[i] == MIR_NO_VALUE) {
        return AUDIO_VALUE_NULL;
      }
    }

    MirValueId result =
        mir_call_value(audio->kernel_builder, result_type, origin,
                       callable.value, callable.type, call_args, arg_count);
    if (result == MIR_NO_VALUE) {
      return AUDIO_VALUE_NULL;
    }
    if (lanes == 1) {
      return audio_mir_wrap_mir_value(audio, origin, result_type, result);
    }
    results[lane] = result;
  }

  return audio_mir_multi_value_typed(audio, origin, origin->type, result_type,
                                     results, lanes);
}

static AudioValue
audio_mir_apply_partial_builtin_values(AudioCompileCtx *audio, Ast *origin,
                                       AudioPartialBuiltin *partial,
                                       AudioValue *args, size_t arg_count) {
  if (!audio || !origin || !partial || !partial->name) {
    return AUDIO_VALUE_NULL;
  }

  int arity = audio_mir_builtin_arity(audio, partial->name);
  size_t total_argc = partial->argc + arg_count;
  if (arity <= 0 || total_argc > (size_t)arity) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue *all_args =
      total_argc
          ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * total_argc,
                            __alignof__(AudioValue))
          : NULL;
  if (total_argc && !all_args) {
    return AUDIO_VALUE_NULL;
  }
  for (size_t i = 0; i < partial->argc; i++) {
    all_args[i] = partial->args[i];
  }
  for (size_t i = 0; i < arg_count; i++) {
    all_args[partial->argc + i] = args[i];
  }

  if (total_argc < (size_t)arity) {
    AudioPartialBuiltin *extended =
        mir_arena_alloc(audio->arena, sizeof(AudioPartialBuiltin),
                        __alignof__(AudioPartialBuiltin));
    if (!extended) {
      return AUDIO_VALUE_NULL;
    }
    *extended = (AudioPartialBuiltin){
        .name = partial->name,
        .type = origin->type,
        .argc = total_argc,
        .args = all_args,
    };
    return (AudioValue){
        .kind = AUDIO_VALUE_PARTIAL_BUILTIN,
        .type = origin->type,
        .value = MIR_NO_VALUE,
        .lanes = 0,
        .vec = NULL,
        .synth = NULL,
        .partial_synth = NULL,
        .partial = extended,
    };
  }

  return audio_mir_emit_builtin_values(audio, origin, partial->name, all_args,
                                       total_argc);
}

static AudioValue audio_mir_apply_audio_value(AudioCompileCtx *audio,
                                              Ast *origin, AudioValue callable,
                                              AudioValue *args,
                                              size_t arg_count,
                                              const char *instance_prefix) {
  switch (callable.kind) {
  case AUDIO_VALUE_SYNTH:
    return audio_mir_apply_audio_synth(audio, origin, callable.synth, NULL, 0,
                                       args, arg_count, instance_prefix);
  case AUDIO_VALUE_PARTIAL_SYNTH:
    return audio_mir_apply_audio_synth(
        audio, origin, callable.partial_synth->synth,
        callable.partial_synth->args, callable.partial_synth->argc, args,
        arg_count, instance_prefix);
  case AUDIO_VALUE_PARTIAL_BUILTIN:
    return audio_mir_apply_partial_builtin_values(
        audio, origin, callable.partial, args, arg_count);
  case AUDIO_VALUE_MIR:
    return audio_mir_apply_mir_callable_value(audio, origin, callable, args,
                                              arg_count);
  default:
    return AUDIO_VALUE_NULL;
  }
}

static AudioValue
audio_mir_emit_synth_in_audio_context(AudioCompileCtx *audio, Ast *app,
                                      MirAudioSynthSymbol *synth) {
  if (!audio || !app || !synth) {
    return AUDIO_VALUE_NULL;
  }

  size_t argc = audio_mir_application_value_arg_count(app);
  AudioValue *args =
      argc ? mir_arena_alloc(audio->arena, sizeof(AudioValue) * argc,
                             __alignof__(AudioValue))
           : NULL;
  if (argc && !args) {
    return AUDIO_VALUE_NULL;
  }
  for (size_t i = 0; i < argc; i++) {
    args[i] = audio_mir_expr(audio, app->data.AST_APPLICATION.args + i);
    if (!audio_mir_value_is_valid(args[i])) {
      return AUDIO_VALUE_NULL;
    }
  }

  AudioValue callable =
      audio_mir_synth_value(app->data.AST_APPLICATION.function
                                ? app->data.AST_APPLICATION.function->type
                                : app->type,
                            synth);
  return audio_mir_apply_audio_value(audio, app, callable, args, argc, NULL);
}

static MirValueId
audio_mir_call_synth_in_audio_context(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirAudioSynthSymbol *synth) {
  (void)builder;
  if (!audio_mir_in_audio_context(ctx)) {
    return MIR_NO_VALUE;
  }

  AudioCompileCtx *audio = (AudioCompileCtx *)ctx->extension_data;
  AudioValue value = audio_mir_emit_synth_in_audio_context(audio, app, synth);
  return audio_mir_value_is_valid(value) ? value.value : MIR_NO_VALUE;
}

static MirValueId MirAudioSynthSymbolHandler(MirBuilder *builder, Ast *app,
                                             MirCtx *ctx, MirSymbol *symbol) {

  MirAudioSynthSymbol *synth = symbol->as.custom.data;
  if (audio_mir_in_audio_context(ctx)) {
    return audio_mir_call_synth_in_audio_context(builder, app, ctx, synth);
  }
  return audio_mir_call_synth_outside_context(builder, app, ctx, synth);
}

static bool audio_mir_bind_synth_symbol(MirAudioSynthBuildCtx *ctx,
                                        MirCtx *mir_ctx) {
  if (!ctx || !ctx->arena || !mir_ctx || !ctx->name || !ctx->ctor_fn ||
      !ctx->init_fn || !ctx->kernel_fn || !ctx->frame_fn) {
    return false;
  }

  MirAudioSynthSymbol *synth =
      audio_mir_make_synth_symbol(ctx->arena, ctx, AUDIO_SYNTH_EXPORTED);
  if (!synth) {
    return false;
  }

  return mir_ctx_bind_export_custom_symbol(
      ctx->program, mir_ctx, ctx->name, ctx->app ? ctx->app->type : NULL,
      ctx->app, MirAudioSynthSymbolHandler, synth);
}

static MirValueId MirCompileAudioHandler(MirBuilder *builder, Ast *app,
                                         MirCtx *ctx,
                                         MirBuiltinSymbol *symbol) {
  if (!builder || !builder->program || !app || app->tag != AST_APPLICATION ||
      app->data.AST_APPLICATION.len < 1) {
    return MIR_NO_VALUE;
  }

  Ast *source = app->data.AST_APPLICATION.args;
  Ast *audio_lambda = audio_mir_source_lambda(source);
  if (!audio_lambda) {
    fprintf(stderr, "audio_jit: @Audio expects a lambda\n");
    return MIR_NO_VALUE;
  }

  MirArena *arena = audio_mir_bundle_arena(builder);
  MirAudioSynthBuildCtx audio_ctx = {
      .program = builder->program,
      .arena = arena,
      .app = app,
      .lambda = audio_lambda,
      .parent_ctx = ctx,
      .name = audio_mir_lambda_name(arena, audio_lambda, source),
      .num_inputs = audio_mir_lambda_input_count(audio_lambda),
      .state_bytes = 0,
  };

  if (!audio_mir_build_synth_functions(&audio_ctx)) {
    fprintf(stderr, "audio_jit: failed to build MIR synth function stubs\n");
    return MIR_NO_VALUE;
  }

  audio_mir_emit_synth_stubs(&audio_ctx);
  if (!audio_mir_bind_synth_symbol(&audio_ctx, ctx)) {
    fprintf(stderr, "audio_jit: failed to bind MIR synth symbol\n");
    return MIR_NO_VALUE;
  }

  return mir_fn_ref(builder, app->type ? app->type : audio_ctx.ctor_fn->type,
                    app, audio_ctx.ctor_fn);
}

static void audio_jit_init_wavetables(void) {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  // maketable_sin();
  initialized = true;
}

static void audio_jit_register_osc_kernel_bitcode(void) {
  const char *path = "libs/audio_jit/build/osc_kernels.bc";

  if (ylc_mir_program && !mir_program_add_llvm_bitcode(ylc_mir_program, path)) {
    fprintf(stderr, "audio_jit: failed to register LLVM bitcode '%s'\n", path);
  }

  if (ylc_jit_module && !ylc_link_llvm_bitcode_file(ylc_jit_module, path)) {
    fprintf(stderr, "audio_jit: failed to link LLVM bitcode '%s'\n", path);
  }
}

static void audio_mir_register_node_builtin(TypeEnv *tenv) {
  if (!audio_mir_osc_node_ctor_name(tenv ? tenv->name : NULL)) {
    return;
  }

  mir_register_builtin(ylc_mir_program, tenv, MirCompileAudioNodeBuiltinHandler,
                       MIR_BUILTIN_SYMBOL_EXTENSION,
                       (MirOperandUse[]){MIR_OPERAND_USE_BORROW}, 1,
                       MIR_RESULT_OWNED);
}

__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  // audio_jit_init_wavetables();
  audio_jit_register_osc_kernel_bitcode();

  if (!ylc_mir_program || !ylc_mir_ctx) {
    return;
  }

  TypeEnv audio_tenv = {.name = "Audio"};

  for (TypeEnv *tenv = ylc_mir_ctx->env; tenv; tenv = tenv->next) {
    audio_mir_register_node_builtin(tenv);
  }

  mir_register_builtin(ylc_mir_program, &audio_tenv, MirCompileAudioHandler,
                       MIR_BUILTIN_SYMBOL_EXTENSION, (MirOperandUse[]){}, 0,
                       MIR_RESULT_NONE);
}
