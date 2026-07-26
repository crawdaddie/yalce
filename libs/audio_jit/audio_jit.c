#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../engine/osc.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/ylc_datatypes.h"
#include "./osc_kernels.h"
#include "mir/mir.h"
#include "serde.h"

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
  return node_raw ? (void *)((Node *)node_raw + 1) : NULL;
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
  node->kind = NODE_KIND_FRAME;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
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

typedef struct MirAudioSynthSymbol {
  const char *name;
  int num_inputs;
  int state_bytes;
  MirFunction *ctor_fn;
  MirFunction *init_fn;
  MirFunction *kernel_fn;
  MirFunction *frame_fn;
} MirAudioSynthSymbol;

typedef struct AudioValue AudioValue;

typedef enum AudioValueKind {
  AUDIO_VALUE_NONE,
  AUDIO_VALUE_MIR,
  AUDIO_VALUE_PARTIAL_BUILTIN,
} AudioValueKind;

typedef struct AudioPartialBuiltin {
  const char *name;
  Type *type;
  size_t argc;
  AudioValue *args;
} AudioPartialBuiltin;

struct AudioValue {
  AudioValueKind kind;
  Type *type;
  MirValueId value;
  int lanes;
  MirValueId *vec;
  AudioPartialBuiltin *partial;
};

typedef struct AudioStateSlot {
  size_t offset;
  size_t size;
  size_t align;
  Type *type;
  const char *name;
  struct AudioStateSlot *next;
} AudioStateSlot;

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
  AudioLocalBinding *locals;
} AudioCompileCtx;

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

static MirValueId MirCompileAudioNodeBuiltinHandler(MirBuilder *builder,
                                                    Ast *app, MirCtx *ctx,
                                                    MirBuiltinSymbol *symbol);

#define AUDIO_VALUE_NULL                                                       \
  (AudioValue) {                                                               \
    .kind = AUDIO_VALUE_NONE, .type = NULL, .value = MIR_NO_VALUE,             \
    .lanes = 0, .vec = NULL, .partial = NULL                                   \
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
  size_t kernel_param_index = 4;
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
  audio->bundle_fns[0] = bundle->ctor_fn;
  audio->bundle_fns[1] = bundle->init_fn;
  audio->bundle_fns[2] = bundle->kernel_fn;
  audio->bundle_fns[3] = bundle->frame_fn;
  audio->bundle_fns_len = 4;
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
      .partial = NULL,
  };
}

static bool audio_mir_value_is_valid(AudioValue value) {
  return value.kind == AUDIO_VALUE_PARTIAL_BUILTIN ||
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

static MirValueId *audio_mir_alloc_lane_values(AudioCompileCtx *audio,
                                               int lanes) {
  if (!audio || !audio->arena || lanes <= 0) {
    return NULL;
  }
  return mir_arena_alloc(audio->arena, sizeof(MirValueId) * (size_t)lanes,
                         __alignof__(MirValueId));
}

static Type *audio_mir_tuple_type(AudioCompileCtx *audio, Type *preferred,
                                  int lanes) {
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
    items[i] = &t_num;
  }
  memset(type, 0, sizeof(*type));
  type->kind = T_CONS;
  type->data.T_CONS.name = TYPE_NAME_TUPLE;
  type->data.T_CONS.args = items;
  type->data.T_CONS.num_args = lanes;
  return type;
}

static AudioValue audio_mir_multi_value(AudioCompileCtx *audio, Ast *origin,
                                        Type *type, MirValueId *values,
                                        int lanes) {
  if (!values || lanes <= 0) {
    return AUDIO_VALUE_NULL;
  }
  if (lanes == 1) {
    return audio_mir_value(&t_num, values[0], 1);
  }

  MirValueIdVec items = {0};
  for (int i = 0; i < lanes; i++) {
    mir_value_id_vec_push(audio->arena, &items, values[i]);
  }
  Type *tuple_type = audio_mir_tuple_type(audio, type, lanes);
  MirValueId tuple =
      mir_tuple(audio->kernel_builder, tuple_type, origin, items);
  return (AudioValue){
      .kind = AUDIO_VALUE_MIR,
      .type = tuple_type,
      .value = tuple,
      .lanes = lanes,
      .vec = values,
      .partial = NULL,
  };
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

  AudioLocalBinding *local =
      mir_arena_alloc(audio->arena, sizeof(AudioLocalBinding),
                      __alignof__(AudioLocalBinding));
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

static AudioValue audio_mir_mir_expr(AudioCompileCtx *audio, Ast *ast) {
  if (!audio || !audio->kernel_builder || !ast) {
    return AUDIO_VALUE_NULL;
  }

  MirValueId value = mir_expr(audio->kernel_builder, ast, audio->mir_ctx);
  if (value == MIR_NO_VALUE) {
    return AUDIO_VALUE_NULL;
  }

  Type *type = ast->type ? ast->type : &t_num;
  if (audio_mir_is_tuple_type(type) && type->data.T_CONS.args &&
      type->data.T_CONS.num_args > 0) {
    int lanes = type->data.T_CONS.num_args;
    MirValueId *values = audio_mir_alloc_lane_values(audio, lanes);
    if (values) {
      for (int i = 0; i < lanes; i++) {
        Type *field_type =
            type->data.T_CONS.args[i] ? type->data.T_CONS.args[i] : &t_num;
        values[i] = mir_tuple_get(audio->kernel_builder, field_type, ast, value,
                                  (size_t)i);
      }
      return (AudioValue){
          .kind = AUDIO_VALUE_MIR,
          .type = type,
          .value = value,
          .lanes = lanes,
          .vec = values,
          .partial = NULL,
      };
    }
  }

  return audio_mir_value(type, value, 1);
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

static bool audio_mir_normalize_num_kernel_args(AudioCompileCtx *audio,
                                                Ast *origin,
                                                const AudioValue *values,
                                                AudioMirKernelArgLanes *args,
                                                size_t argc, int *out_lanes) {
  if (!audio || !origin || !values || !args || argc == 0 || !out_lanes) {
    return false;
  }

  int lanes = audio_mir_max_lanes(values, argc);
  if (lanes <= 0) {
    return false;
  }

  for (size_t i = 0; i < argc; i++) {
    MirValueId *lane_values = audio_mir_alloc_lane_values(audio, lanes);
    if (!lane_values) {
      return false;
    }

    for (int lane = 0; lane < lanes; lane++) {
      lane_values[lane] = audio_mir_num_lane(audio, origin, values[i], lane);
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

static bool audio_mir_is_primitive_type(Type *type) {
  return type && type->kind <= T_STRING;
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

  size_t argc = app->data.AST_APPLICATION.len;
  AudioValue *args = argc ? mir_arena_alloc(audio->arena,
                                            sizeof(AudioValue) * argc,
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

    MirValueId result =
        mir_call_value(b, result_type, app, callee, callee_type, call_args,
                       argc);
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

static int audio_mir_builtin_arity(const char *name) {
  if (!name) {
    return 0;
  }
  if (strcmp(name, "sin_osc") == 0 || strcmp(name, "sq_osc") == 0 ||
      strcmp(name, "saw_osc") == 0) {
    return 1;
  }
  if (strcmp(name, "pm_osc") == 0) {
    return 3;
  }
  return 0;
}

static AudioValue audio_mir_emit_sin_osc_value(AudioCompileCtx *audio,
                                               Ast *origin,
                                               AudioValue freq) {
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
                                               Ast *origin,
                                               AudioValue freq) {
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

static AudioValue audio_mir_emit_builtin_values(AudioCompileCtx *audio,
                                                Ast *origin, const char *name,
                                                AudioValue *args,
                                                size_t argc) {
  if (!name || !args) {
    return AUDIO_VALUE_NULL;
  }
  if (strcmp(name, "sin_osc") == 0 && argc == 1) {
    return audio_mir_emit_sin_osc_value(audio, origin, args[0]);
  }
  if (strcmp(name, "sq_osc") == 0 && argc == 1) {
    return audio_mir_emit_sq_osc_value(audio, origin, args[0]);
  }
  if (strcmp(name, "saw_osc") == 0 && argc == 1) {
    return audio_mir_emit_saw_osc_value(audio, origin, args[0]);
  }
  if (strcmp(name, "pm_osc") == 0 && argc == 3) {
    return audio_mir_emit_pm_osc_values(audio, origin, args[0], args[1],
                                        args[2]);
  }
  return AUDIO_VALUE_NULL;
}

static AudioValue audio_mir_make_partial_builtin(AudioCompileCtx *audio,
                                                 Ast *app,
                                                 const char *name) {
  int arity = audio_mir_builtin_arity(name);
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
      .partial = partial,
  };
}

static AudioValue audio_mir_apply_partial_builtin(AudioCompileCtx *audio,
                                                  Ast *app,
                                                  AudioPartialBuiltin *partial) {
  if (!audio || !app || !partial || !partial->name) {
    return AUDIO_VALUE_NULL;
  }

  int arity = audio_mir_builtin_arity(partial->name);
  size_t new_argc = app->data.AST_APPLICATION.len;
  size_t total_argc = partial->argc + new_argc;
  if (arity <= 0 || total_argc > (size_t)arity) {
    return AUDIO_VALUE_NULL;
  }

  AudioValue *args =
      mir_arena_alloc(audio->arena, sizeof(AudioValue) * total_argc,
                      __alignof__(AudioValue));
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
        .partial = extended,
    };
  }

  return audio_mir_emit_builtin_values(audio, app, partial->name, args,
                                       total_argc);
}

static AudioValue SinOscBuiltin(AudioCompileCtx *audio, Ast *app) {
  AudioValue freq = audio_mir_expr(audio, app->data.AST_APPLICATION.args);
  return audio_mir_emit_sin_osc_value(audio, app, freq);
}

static AudioValue SqOscBuiltin(AudioCompileCtx *audio, Ast *app) {
  AudioValue freq = audio_mir_expr(audio, app->data.AST_APPLICATION.args);
  return audio_mir_emit_sq_osc_value(audio, app, freq);
}

static AudioValue SawOscBuiltin(AudioCompileCtx *audio, Ast *app) {
  AudioValue freq = audio_mir_expr(audio, app->data.AST_APPLICATION.args);
  return audio_mir_emit_saw_osc_value(audio, app, freq);
}

static AudioValue PmOscBuiltin(AudioCompileCtx *audio, Ast *app) {
  AudioValue freq = audio_mir_expr(audio, app->data.AST_APPLICATION.args + 2);

  AudioValue mod_index =
      audio_mir_expr(audio, app->data.AST_APPLICATION.args + 1);

  AudioValue mod_ratio = audio_mir_expr(audio, app->data.AST_APPLICATION.args);

  return audio_mir_emit_pm_osc_values(audio, app, mod_ratio, mod_index, freq);
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
  case AST_IDENTIFIER: {
    AudioValue *local =
        audio_mir_lookup_local(audio, ast->data.AST_IDENTIFIER.value);
    return local ? *local : audio_mir_mir_expr(audio, ast);
  }
  case AST_APPLICATION: {
    ast = audio_mir_application_flatten_if_saturated(audio, ast);
    const char *name = audio_mir_application_name(ast);

    if (name && audio_mir_application_is_partial(ast) &&
        audio_mir_builtin_arity(name) > 0) {
      return audio_mir_make_partial_builtin(audio, ast, name);
    }

    if (audio_mir_application_is_partial(ast)) {
      return audio_mir_mir_expr(audio, ast);
    }

    AudioValue *local = name ? audio_mir_lookup_local(audio, name) : NULL;
    if (local && local->kind == AUDIO_VALUE_PARTIAL_BUILTIN) {
      return audio_mir_apply_partial_builtin(audio, ast, local->partial);
    }

    if (!name && ast->data.AST_APPLICATION.function) {
      AudioValue callee =
          audio_mir_expr(audio, ast->data.AST_APPLICATION.function);
      if (callee.kind == AUDIO_VALUE_PARTIAL_BUILTIN) {
        return audio_mir_apply_partial_builtin(audio, ast, callee.partial);
      }
    }

    if (name) {
      if (strcmp(name, "~") == 0) {
        return multichannel_operator(audio, ast);
      }

      if (strcmp(name, "sin_osc") == 0) {
        return SinOscBuiltin(audio, ast);
      }

      if (strcmp(name, "sq_osc") == 0) {
        return SqOscBuiltin(audio, ast);
      }

      if (strcmp(name, "saw_osc") == 0) {
        return SawOscBuiltin(audio, ast);
      }

      if (strcmp(name, "pm_osc") == 0) {
        return PmOscBuiltin(audio, ast);
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
    MirValueId ptr =
        audio_mir_state_slot_ptr(audio, audio->init_builder, audio->app, state,
                                 slot->offset, slot->type);
    MirValueId zero =
        audio_mir_zero_value(audio->init_builder, slot->type, audio->app);
    if (ptr != MIR_NO_VALUE && zero != MIR_NO_VALUE) {
      mir_ptr_store(audio->init_builder, audio->app, ptr, zero);
    }
  }

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
        audio_mir_extern_ref(b, "plug_input_in_graph", plug_type, audio->app);
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
  return opt->cons_fn && opt->init_fn && opt->kernel_fn && opt->frame_fn;
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

static MirValueId audio_mir_call_synth_constructor(MirBuilder *builder,
                                                   Ast *app, MirCtx *ctx,
                                                   MirAudioSynthSymbol *synth) {
  if (!builder || !builder->program || !builder->fn || !app || !synth ||
      !synth->ctor_fn) {
    return MIR_NO_VALUE;
  }

  size_t argc = (size_t)app->data.AST_APPLICATION.len;
  MirValueId *args =
      argc ? mir_arena_alloc(builder->fn->arena, sizeof(MirValueId) * argc,
                             __alignof__(MirValueId))
           : NULL;
  if (argc && !args) {
    return MIR_NO_VALUE;
  }

  for (size_t i = 0; i < argc; i++) {
    args[i] = mir_expr(builder, app->data.AST_APPLICATION.args + i, ctx);
    if (args[i] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
  }

  app->type = &t_ptr;
  MirValueId ctor =
      mir_fn_ref(builder, synth->ctor_fn->type, app, synth->ctor_fn);
  return mir_call_value(builder, &t_ptr, app, ctor, synth->ctor_fn->type, args,
                        argc);
}

static MirValueId
audio_mir_call_synth_in_audio_context(MirBuilder *builder, Ast *app,
                                      MirCtx *ctx, MirAudioSynthSymbol *synth) {
  if (!builder || !builder->program || !builder->fn || !app || !synth ||
      !synth->kernel_fn) {
    return MIR_NO_VALUE;
  }

  AudioCompileCtx *audio = audio_mir_in_audio_context(ctx)
                               ? (AudioCompileCtx *)ctx->extension_data
                               : NULL;
  size_t argc = (size_t)app->data.AST_APPLICATION.len;
  size_t total_argc = argc + 4;
  MirValueId *args =
      total_argc
          ? mir_arena_alloc(builder->fn->arena, sizeof(MirValueId) * total_argc,
                            __alignof__(MirValueId))
          : NULL;
  if (!args) {
    return MIR_NO_VALUE;
  }

  MirValueId node = audio && audio->node_param != MIR_NO_VALUE
                        ? audio->node_param
                        : MIR_NO_VALUE;
  if (node == MIR_NO_VALUE && !mir_ctx_lookup_value(ctx, "node", &node)) {
    node = mir_const_undef(builder, &t_ptr, app);
  }

  Type *state_type =
      synth->kernel_fn->params.len > 1 && synth->kernel_fn->params.items[1].type
          ? synth->kernel_fn->params.items[1].type
          : (audio && audio->ptr_char_type ? audio->ptr_char_type : &t_ptr);
  MirValueId state = MIR_NO_VALUE;
  if (audio && audio->state_param != MIR_NO_VALUE) {
    size_t state_bytes =
        synth->state_bytes > 0 ? (size_t)synth->state_bytes : 1;
    AudioStateSlot *slot =
        audio_mir_reserve_state_slot(audio, &t_char, state_bytes, 8,
                                     synth->name ? synth->name : "sub_synth");
    if (slot) {
      state = audio_mir_state_slot_ptr(audio, builder, app, audio->state_param,
                                       slot->offset, &t_char);
      if (state != MIR_NO_VALUE && state_type != audio->ptr_char_type) {
        state = mir_primitive_cast(builder, audio->ptr_char_type, state_type,
                                   app, state);
      }
    }
  }
  if (state == MIR_NO_VALUE) {
    state = mir_const_undef(builder, state_type, app);
  }
  if (node == MIR_NO_VALUE || state == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  args[0] = node;
  args[1] = state;
  args[2] = audio && audio->frame_param != MIR_NO_VALUE
                ? audio->frame_param
                : mir_const_undef(builder, &t_int, app);
  args[3] = audio && audio->spf_param != MIR_NO_VALUE
                ? audio->spf_param
                : mir_const_undef(builder, &t_num, app);
  for (size_t i = 0; i < argc; i++) {
    args[i + 4] = mir_expr(builder, app->data.AST_APPLICATION.args + i, ctx);
    if (args[i + 4] == MIR_NO_VALUE) {
      return MIR_NO_VALUE;
    }
  }

  Type *result_type = fn_return_type(synth->kernel_fn->type);
  if (!result_type) {
    result_type = app->type ? app->type : &t_num;
  }
  app->type = result_type;
  MirValueId kernel =
      mir_fn_ref(builder, synth->kernel_fn->type, app, synth->kernel_fn);
  return mir_call_value(builder, result_type, app, kernel,
                        synth->kernel_fn->type, args, total_argc);
}

static MirValueId MirAudioSynthSymbolHandler(MirBuilder *builder, Ast *app,
                                             MirCtx *ctx, MirSymbol *symbol) {

  MirAudioSynthSymbol *synth = symbol->as.custom.data;
  if (audio_mir_in_audio_context(ctx)) {
    return audio_mir_call_synth_in_audio_context(builder, app, ctx, synth);
  }
  return audio_mir_call_synth_constructor(builder, app, ctx, synth);
}

static bool audio_mir_bind_synth_symbol(MirAudioSynthBuildCtx *ctx,
                                        MirCtx *mir_ctx) {
  if (!ctx || !ctx->arena || !mir_ctx || !ctx->name || !ctx->ctor_fn ||
      !ctx->init_fn || !ctx->kernel_fn || !ctx->frame_fn) {
    return false;
  }

  MirAudioSynthSymbol *synth =
      mir_arena_alloc(ctx->arena, sizeof(MirAudioSynthSymbol),
                      __alignof__(MirAudioSynthSymbol));
  if (!synth) {
    return false;
  }
  *synth = (MirAudioSynthSymbol){
      .name = ctx->name,
      .num_inputs = ctx->num_inputs,
      .state_bytes = ctx->state_bytes,
      .ctor_fn = ctx->ctor_fn,
      .init_fn = ctx->init_fn,
      .kernel_fn = ctx->kernel_fn,
      .frame_fn = ctx->frame_fn,
  };

  return mir_ctx_bind_custom_symbol(mir_ctx, ctx->name,
                                    ctx->app ? ctx->app->type : NULL, ctx->app,
                                    MirAudioSynthSymbolHandler, synth);
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
