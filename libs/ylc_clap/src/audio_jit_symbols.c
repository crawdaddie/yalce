#include "audio_jit_symbols.h"

#include "audio_jit.h"
#include "types/builtins.h"
#include "types/type.h"

#include <stdio.h>
#include <string.h>

static Type *ylc_clap_mir_fn_type(MirArena *arena, Type **params,
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

static MirFunction *ylc_clap_mir_extern_fn(MirBuilder *builder,
                                           const char *name, Type *type,
                                           Ast *origin) {
  if (!builder || !builder->program || !name || !type) {
    return NULL;
  }
  return mir_program_add_extern_function(builder->program, name, type, origin);
}

static MirValueId ylc_clap_mir_extern_ref(MirBuilder *builder,
                                          const char *name, Type *type,
                                          Ast *origin) {
  MirFunction *fn = ylc_clap_mir_extern_fn(builder, name, type, origin);
  return fn ? mir_fn_ref(builder, type, origin, fn) : MIR_NO_VALUE;
}

static MirFunction *ylc_clap_build_play_pattern_step(MirBuilder *builder,
                                                     Ast *app,
                                                     Type *coro_type) {
  if (!builder || !builder->fn || !is_coroutine_type(coro_type) ||
      !coro_type->data.T_CONS.args || coro_type->data.T_CONS.num_args < 1) {
    return NULL;
  }

  Type *yield_type = coro_type->data.T_CONS.args[0];
  Type *next_type = create_option_type(yield_type);
  Type *some_type = next_type && next_type->data.T_CONS.args
                        ? next_type->data.T_CONS.args[0]
                        : NULL;
  if (!yield_type || !next_type || !some_type) {
    return NULL;
  }

  Type *step_params[] = {coro_type, &t_uint64};
  Type *step_type = ylc_clap_mir_fn_type(
      builder->fn->arena, step_params, sizeof(step_params) / sizeof(step_params[0]),
      &t_void);
  if (!step_type) {
    return NULL;
  }

  static unsigned play_pattern_step_counter = 0;
  char step_name[80];
  snprintf(step_name, sizeof(step_name), "__ylc_clap_play_pattern_step_%u",
           play_pattern_step_counter++);

  MirFunction *step =
      mir_program_add_function(builder->program, step_name, step_type, app);
  if (!step) {
    return NULL;
  }

  MirValueId handle = mir_function_add_param(step, "handle", coro_type, app);
  MirValueId tick = mir_function_add_param(step, "tick", &t_uint64, app);
  if (handle == MIR_NO_VALUE || tick == MIR_NO_VALUE) {
    return NULL;
  }

  MirBlock *entry = mir_function_add_block(step, "entry");
  MirBlock *resume_block = mir_function_add_block(step, "coro.resume");
  MirBlock *value_block = mir_function_add_block(step, "coro.value");
  MirBlock *finished = mir_function_add_block(step, "coro.finished");
  if (!entry || !resume_block || !value_block || !finished) {
    return NULL;
  }

  MirBuilder wb;
  mir_builder_init(&wb, builder->program, step);

  mir_builder_position_at_end(&wb, entry);
  mir_builder_set_br(&wb, resume_block->id);

  mir_builder_position_at_end(&wb, resume_block);
  MirValueId next = mir_coro_next(&wb, app, handle, coro_type);
  MirValueId tag = mir_variant_tag(&wb, app, next);
  MirValueId is_some = mir_tag_eq(&wb, app, tag, 0, TYPE_NAME_SOME);
  if (next == MIR_NO_VALUE || tag == MIR_NO_VALUE || is_some == MIR_NO_VALUE) {
    return NULL;
  }
  mir_builder_set_cond(&wb, is_some, value_block->id, finished->id);

  mir_builder_position_at_end(&wb, value_block);
  MirValueId payload =
      mir_variant_payload(&wb, app, next, some_type, 0, TYPE_NAME_SOME);
  MirValueId yielded = mir_tuple_get(&wb, yield_type, app, payload, 0);
  MirValueId dur = is_tuple_type(yield_type)
                       ? mir_tuple_get(&wb, yield_type, app, yielded, 0)
                       : yielded;
  if (payload == MIR_NO_VALUE || yielded == MIR_NO_VALUE ||
      dur == MIR_NO_VALUE) {
    return NULL;
  }

  Type *sched_params[] = {&t_uint64, &t_num};
  Type *schedule_event_type = ylc_clap_mir_fn_type(
      wb.fn->arena, sched_params, sizeof(sched_params) / sizeof(sched_params[0]),
      &t_ptr);
  MirValueId schedule_event_fn = ylc_clap_mir_extern_ref(
      &wb, "ylc_clap_schedule_current_task_event", schedule_event_type, app);
  if (schedule_event_fn == MIR_NO_VALUE) {
    return NULL;
  }
  MirValueId sched_args[] = {tick, dur};
  mir_call_value(&wb, &t_ptr, app, schedule_event_fn, schedule_event_type,
                 sched_args, sizeof(sched_args) / sizeof(sched_args[0]));
  mir_builder_set_return(&wb, mir_const_void(&wb, &t_void, app));

  mir_builder_position_at_end(&wb, finished);
  Type *complete_type = ylc_clap_mir_fn_type(wb.fn->arena, NULL, 0, &t_void);
  MirValueId complete_fn = ylc_clap_mir_extern_ref(
      &wb, "ylc_clap_complete_current_task", complete_type, app);
  if (complete_fn != MIR_NO_VALUE) {
    mir_call_value(&wb, &t_void, app, complete_fn, complete_type, NULL, 0);
  }
  mir_builder_set_return(&wb, mir_const_void(&wb, &t_void, app));

  return step;
}

static MirValueId ylc_clap_play_pattern_handler(MirBuilder *builder, Ast *app,
                                                MirCtx *ctx,
                                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!builder || !app || app->tag != AST_APPLICATION ||
      app->data.AST_APPLICATION.len < 2) {
    return MIR_NO_VALUE;
  }

  Ast *quant_ast = app->data.AST_APPLICATION.args;
  Ast *coro_ast = app->data.AST_APPLICATION.args + 1;
  MirValueId quant = mir_expr(builder, quant_ast, ctx);
  MirValueId cor = mir_expr(builder, coro_ast, ctx);
  if (quant == MIR_NO_VALUE || cor == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  Type *coro_type =
      is_coroutine_type(coro_ast->type) ? coro_ast->type : app->type;
  if (!is_coroutine_type(coro_type)) {
    return MIR_NO_VALUE;
  }

  MirFunction *step =
      ylc_clap_build_play_pattern_step(builder, app, coro_type);
  if (!step) {
    return MIR_NO_VALUE;
  }

  Type *start_params[] = {&t_num, &t_ptr, coro_type};
  Type *start_type = ylc_clap_mir_fn_type(
      builder->fn->arena, start_params,
      sizeof(start_params) / sizeof(start_params[0]), &t_ptr);
  MirFunction *start_extern =
      ylc_clap_mir_extern_fn(builder, "ylc_play_pattern_start", start_type, app);
  if (start_extern && start_extern->summary.param_uses.len >= 3) {
    start_extern->summary.param_uses.items[2] = MIR_OPERAND_USE_CONSUME;
  }
  MirValueId start_fn =
      start_extern ? mir_fn_ref(builder, start_type, app, start_extern)
                   : MIR_NO_VALUE;
  MirValueId step_ref = mir_fn_ref(builder, step->type, app, step);
  if (start_fn == MIR_NO_VALUE || step_ref == MIR_NO_VALUE) {
    return MIR_NO_VALUE;
  }

  MirValueId start_args[] = {quant, step_ref, cor};
  return mir_call_value(builder, &t_ptr, app, start_fn, start_type, start_args,
                        sizeof(start_args) / sizeof(start_args[0]));
}

static MirValueId ylc_clap_alloc_voices_handler(MirBuilder *builder, Ast *app,
                                                MirCtx *ctx,
                                                MirBuiltinSymbol *symbol) {
  (void)symbol;
  if (!builder || !app || app->tag != AST_APPLICATION ||
      app->data.AST_APPLICATION.len != 2) {
    return MIR_NO_VALUE;
  }

  return ylc_audio_jit_emit_synth_voice_array(
      builder, app, ctx, app->data.AST_APPLICATION.args,
      app->data.AST_APPLICATION.args + 1);
}

void ylc_clap_register_audio_jit_symbols(MirProgram *program, MirCtx *ctx) {
  (void)ctx;
  if (!program) {
    return;
  }

  static const ylc_audio_jit_runtime_arg_t param_args[] = {
      {YLC_AUDIO_JIT_RUNTIME_ARG_CONST_INT, 0},
  };
  ylc_audio_jit_register_runtime_double_builtin(
      &(ylc_audio_jit_runtime_builtin_desc_t){
          .name = "param",
          .source_argc = 1,
          .runtime_symbol = "ylc_plugin_param_value",
          .runtime_args = param_args,
          .runtime_argc = sizeof(param_args) / sizeof(param_args[0]),
      });
  ylc_audio_jit_register_runtime_double_builtin(
      &(ylc_audio_jit_runtime_builtin_desc_t){
          .name = "tempo_mul",
          .source_argc = 1,
          .runtime_symbol = "ylc_plugin_tempo_mul",
          .runtime_args = NULL,
          .runtime_argc = 0,
      });
  ylc_audio_jit_register_runtime_double_builtin(
      &(ylc_audio_jit_runtime_builtin_desc_t){
          .name = "tempo_coeff",
          .source_argc = 1,
          .runtime_symbol = "ylc_plugin_tempo_mul",
          .runtime_args = NULL,
          .runtime_argc = 0,
      });

  TypeEnv play_tenv = {.name = "play_pattern"};
  mir_register_builtin(program, &play_tenv, ylc_clap_play_pattern_handler,
                       MIR_BUILTIN_SYMBOL_EXTENSION, NULL, 0, MIR_RESULT_NONE);

  TypeEnv alloc_voices_tenv = {.name = "alloc_voices"};
  mir_register_builtin(program, &alloc_voices_tenv,
                       ylc_clap_alloc_voices_handler,
                       MIR_BUILTIN_SYMBOL_EXTENSION, NULL, 0,
                       MIR_RESULT_OWNED);
}
