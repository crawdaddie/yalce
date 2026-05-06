#include "./dsp_rubber_band.h"

#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/ylc_datatypes.h"
#include "./audio_jit.h"

#include "../rubberband/rubberband/rubberband-c.h"

#include <llvm-c/Core.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  RubberBandState rb;
  int sample_rate;
  int channels;
  int block_frames;
  int output_capacity_frames;
  int finer_engine;
  uint64_t source_frame;
  double prev_trig;
  double *frame_cache;
  float *input_planar;
  float *output_planar;
  const float **input_channels;
  float **output_channels;
  int output_frames;
  int output_index;
} DspRubberBandRuntimeState;

static int rb_align_i32(int value, int align) {
  return (value + align - 1) & ~(align - 1);
}

static char *rb_align_ptr(char *ptr, size_t align) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t aligned = (p + align - 1u) & ~(uintptr_t)(align - 1u);
  return (char *)aligned;
}

static inline LLVMValueRef rb_dsp_value_lane(DspValue v, int i) {
  return v.lanes > 1 ? v.vec[i % v.lanes] : v.scalar;
}

static int rb_try_eval_const_num(Ast *ast, DspBuildCtx *dsp_ctx, double *out) {
  if (!ast || !out) {
    return 0;
  }

  switch (ast->tag) {
  case AST_INT:
    *out = (double)ast->data.AST_INT.value;
    return 1;
  case AST_DOUBLE:
    *out = ast->data.AST_DOUBLE.value;
    return 1;
  case AST_IDENTIFIER:
    if (strcmp(ast->data.AST_IDENTIFIER.value, "sample_rate") == 0) {
      *out = (double)dsp_ctx->sample_rate;
      return 1;
    }
    return 0;
  default:
    return 0;
  }
}

static int rb_output_capacity_frames_for_block(int block_frames) {
  int capacity = block_frames * 4;
  if (capacity < block_frames) {
    capacity = block_frames;
  }
  return capacity;
}

int dsp_rubberband_state_bytes_for(int channels, int block_frames) {
  if (channels <= 0 || block_frames <= 0) {
    return 0;
  }

  int bytes = rb_align_i32((int)sizeof(DspRubberBandRuntimeState), 8);
  bytes = rb_align_i32(bytes, 8);
  bytes += (int)(sizeof(double) * (size_t)channels);
  bytes = rb_align_i32(bytes, 8);
  bytes += (int)(sizeof(float) * (size_t)channels * (size_t)block_frames);
  bytes = rb_align_i32(bytes, 8);
  bytes += (int)(sizeof(float) * (size_t)channels *
                 (size_t)rb_output_capacity_frames_for_block(block_frames));
  bytes = rb_align_i32(bytes, 8);
  bytes += (int)(sizeof(const float *) * (size_t)channels);
  bytes = rb_align_i32(bytes, 8);
  bytes += (int)(sizeof(float *) * (size_t)channels);
  return rb_align_i32(bytes, 8);
}

static uint64_t rb_compute_start_frame(DspRubberBandRuntimeState *state,
                                       _DoubleArray buf, double start_pos) {
  if (!state || !buf.data || state->channels <= 0 || buf.size <= 0) {
    return 0;
  }

  int total_frames = buf.size / state->channels;
  if (total_frames <= 0) {
    return 0;
  }

  if (start_pos < 0.0) {
    start_pos = 0.0;
  }
  if (start_pos > 1.0) {
    start_pos = 1.0;
  }

  int start_frame = (int)(start_pos * (double)total_frames);
  if (start_frame >= total_frames) {
    start_frame = total_frames - 1;
  }
  if (start_frame < 0) {
    start_frame = 0;
  }
  return (uint64_t)start_frame;
}

static void rb_runtime_reset_stream(DspRubberBandRuntimeState *state) {
  if (!state || !state->rb) {
    return;
  }
  rubberband_reset(state->rb);
  state->source_frame = 0;
  state->output_frames = 0;
  state->output_index = 0;
  if (state->frame_cache) {
    memset(state->frame_cache, 0, sizeof(double) * (size_t)state->channels);
  }
}

void dsp_rubberband_init(void *state_mem, int sample_rate, int channels,
                         int block_frames, int finer_engine) {
  if (!state_mem || sample_rate <= 0 || channels <= 0 || block_frames <= 0) {
    return;
  }

  DspRubberBandRuntimeState *state = (DspRubberBandRuntimeState *)state_mem;
  memset(state, 0, sizeof(*state));

  state->sample_rate = sample_rate;
  state->channels = channels;
  state->block_frames = block_frames;
  state->output_capacity_frames =
      rb_output_capacity_frames_for_block(block_frames);
  state->finer_engine = finer_engine ? 1 : 0;

  char *cursor = (char *)state_mem;
  cursor += rb_align_i32((int)sizeof(DspRubberBandRuntimeState), 8);
  cursor = rb_align_ptr(cursor, 8);
  state->frame_cache = (double *)cursor;
  cursor += sizeof(double) * (size_t)channels;
  cursor = rb_align_ptr(cursor, 8);
  state->input_planar = (float *)cursor;
  cursor += sizeof(float) * (size_t)channels * (size_t)block_frames;
  cursor = rb_align_ptr(cursor, 8);
  state->output_planar = (float *)cursor;
  cursor +=
      sizeof(float) * (size_t)channels * (size_t)state->output_capacity_frames;
  cursor = rb_align_ptr(cursor, 8);
  state->input_channels = (const float **)cursor;
  cursor += sizeof(const float *) * (size_t)channels;
  cursor = rb_align_ptr(cursor, 8);
  state->output_channels = (float **)cursor;

  RubberBandOptions options =
      RubberBandOptionProcessRealTime | RubberBandOptionThreadingNever |
      RubberBandOptionPitchHighConsistency | RubberBandOptionFormantPreserved |
      RubberBandOptionChannelsTogether;
  if (state->finer_engine) {
    options |= RubberBandOptionEngineFiner;
  }

  state->rb = rubberband_new((unsigned int)sample_rate, (unsigned int)channels,
                             options, 1.0, 1.0);
  if (!state->rb) {
    return;
  }

  rubberband_set_max_process_size(state->rb, (unsigned int)block_frames);

  for (int ch = 0; ch < channels; ch++) {
    state->input_channels[ch] =
        state->input_planar + ((size_t)ch * (size_t)block_frames);
    state->output_channels[ch] =
        state->output_planar +
        ((size_t)ch * (size_t)state->output_capacity_frames);
  }

  rb_runtime_reset_stream(state);
  state->prev_trig = 0.0;
}

static void rb_runtime_fill_input_block(DspRubberBandRuntimeState *state,
                                        _DoubleArray buf) {
  int frames = 0;
  if (state->channels > 0) {
    frames = buf.size / state->channels;
  }
  if (frames <= 0) {
    memset(state->input_planar, 0,
           sizeof(float) * (size_t)state->channels *
               (size_t)state->block_frames);
    return;
  }

  for (int frame = 0; frame < state->block_frames; frame++) {
    uint64_t src_frame = state->source_frame % (uint64_t)frames;
    for (int ch = 0; ch < state->channels; ch++) {
      size_t src_idx = (size_t)src_frame * (size_t)state->channels + (size_t)ch;
      size_t dst_idx = (size_t)ch * (size_t)state->block_frames + (size_t)frame;
      state->input_planar[dst_idx] = (float)buf.data[src_idx];
    }
    state->source_frame++;
  }
}

static int rb_runtime_refill_output(DspRubberBandRuntimeState *state,
                                    _DoubleArray buf) {
  if (!state || !state->rb || !buf.data || buf.size <= 0) {
    return 0;
  }

  for (int attempts = 0; attempts < 4; attempts++) {
    rb_runtime_fill_input_block(state, buf);
    rubberband_process(state->rb, state->input_channels,
                       (unsigned int)state->block_frames, 0);

    int available = rubberband_available(state->rb);
    if (available <= 0) {
      continue;
    }
    if (available > state->output_capacity_frames) {
      available = state->output_capacity_frames;
    }

    unsigned int retrieved = rubberband_retrieve(
        state->rb, state->output_channels, (unsigned int)available);
    if (retrieved == 0) {
      continue;
    }

    state->output_frames = (int)retrieved;
    state->output_index = 0;
    return 1;
  }

  return 0;
}

void dsp_rubberband_next_frame(void *state_raw, _DoubleArray buf,
                               double pitch_scale, double time_ratio,
                               double start_pos, double trig) {
  DspRubberBandRuntimeState *state = state_raw;
  if (!state || !state->rb || state->channels <= 0) {
    return;
  }

  if (trig >= 0.5 && state->prev_trig < 0.5) {
    rb_runtime_reset_stream(state);
    state->source_frame = rb_compute_start_frame(state, buf, start_pos);
  }
  state->prev_trig = trig;

  if (time_ratio <= 0.0) {
    time_ratio = 1.0;
  }
  if (pitch_scale <= 0.0) {
    pitch_scale = 1.0;
  }

  rubberband_set_time_ratio(state->rb, time_ratio);
  rubberband_set_pitch_scale(state->rb, pitch_scale);

  if (state->output_index >= state->output_frames) {
    state->output_frames = 0;
    state->output_index = 0;
    if (!rb_runtime_refill_output(state, buf)) {
      memset(state->frame_cache, 0, sizeof(double) * (size_t)state->channels);
      return;
    }
  }

  for (int ch = 0; ch < state->channels; ch++) {
    size_t idx = (size_t)ch * (size_t)state->output_capacity_frames +
                 (size_t)state->output_index;
    state->frame_cache[ch] = (double)state->output_planar[idx];
  }
  state->output_index++;
}

double dsp_rubberband_get_channel(void *state_raw, int channel_idx) {
  DspRubberBandRuntimeState *state = state_raw;
  if (!state || !state->frame_cache || channel_idx < 0 ||
      channel_idx >= state->channels) {
    return 0.0;
  }
  return state->frame_cache[channel_idx];
}

static DspValue build_rubberband_bufplay(LLVMValueRef buf, LLVMValueRef pitch,
                                         LLVMValueRef time_ratio,
                                         LLVMValueRef start_pos,
                                         LLVMValueRef trig, int num_channels,
                                         int finer_engine, DspBuildCtx *dsp_ctx,
                                         LLVMModuleRef module,
                                         LLVMBuilderRef builder) {
  int off = rb_align_i32(dsp_ctx->state_offset, 8);
  int block_frames = 512;
  int state_bytes = dsp_rubberband_state_bytes_for(num_channels, block_frames);
  dsp_ctx->state_offset = off + state_bytes;

  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef ptr_ty = GENERIC_PTR;
  LLVMTypeRef arr_ty =
      LLVMStructType((LLVMTypeRef[]){i32_ty, LLVMPointerType(f64_ty, 0)}, 2, 0);

  if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
    LLVMValueRef init_base = dsp_consume_init_state(
        dsp_ctx, dsp_ctx->init_builder, state_bytes, 8, "rb.init.base");
    LLVMTypeRef init_ty = LLVMFunctionType(
        LLVMVoidType(), (LLVMTypeRef[]){ptr_ty, i32_ty, i32_ty, i32_ty, i32_ty},
        5, 0);
    LLVMValueRef init_fn = LLVMGetNamedFunction(module, "dsp_rubberband_init");
    if (!init_fn) {
      init_fn = LLVMAddFunction(module, "dsp_rubberband_init", init_ty);
      LLVMSetLinkage(init_fn, LLVMExternalLinkage);
    }
    LLVMValueRef init_args[] = {
        init_base,
        LLVMConstInt(i32_ty, (uint64_t)dsp_ctx->sample_rate, 0),
        LLVMConstInt(i32_ty, (uint64_t)num_channels, 0),
        LLVMConstInt(i32_ty, (uint64_t)block_frames, 0),
        LLVMConstInt(i32_ty, (uint64_t)(finer_engine ? 1 : 0), 0),
    };
    LLVMBuildCall2(dsp_ctx->init_builder, init_ty, init_fn, init_args, 5, "");
  }

  LLVMValueRef state_obj =
      dsp_consume_frame_state(dsp_ctx, builder, state_bytes, 8, "rb.base");

  LLVMValueRef buf_struct = buf;
  if (LLVMGetTypeKind(LLVMTypeOf(buf)) == LLVMPointerTypeKind) {
    buf_struct = LLVMBuildLoad2(builder, arr_ty, buf, "rb.buf.arr");
  }

  LLVMTypeRef next_ty = LLVMFunctionType(
      LLVMVoidType(),
      (LLVMTypeRef[]){ptr_ty, arr_ty, f64_ty, f64_ty, f64_ty, f64_ty}, 6, 0);
  LLVMValueRef next_fn =
      LLVMGetNamedFunction(module, "dsp_rubberband_next_frame");
  if (!next_fn) {
    next_fn = LLVMAddFunction(module, "dsp_rubberband_next_frame", next_ty);
    LLVMSetLinkage(next_fn, LLVMExternalLinkage);
  }
  LLVMValueRef next_args[] = {state_obj,  buf_struct, pitch,
                              time_ratio, start_pos,  trig};
  LLVMBuildCall2(builder, next_ty, next_fn, next_args, 6, "");

  LLVMTypeRef get_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){ptr_ty, i32_ty}, 2, 0);
  LLVMValueRef get_fn =
      LLVMGetNamedFunction(module, "dsp_rubberband_get_channel");
  if (!get_fn) {
    get_fn = LLVMAddFunction(module, "dsp_rubberband_get_channel", get_ty);
    LLVMSetLinkage(get_fn, LLVMExternalLinkage);
  }

  if (num_channels <= 1) {
    LLVMValueRef get_args[] = {state_obj, LLVMConstInt(i32_ty, 0, 0)};
    return DSP_SCALAR(
        LLVMBuildCall2(builder, get_ty, get_fn, get_args, 2, "rb.sample"));
  }

  LLVMValueRef *vals =
      dsp_tmp_alloc(dsp_ctx, sizeof(LLVMValueRef) * (size_t)num_channels, 8);
  if (!vals) {
    return DSP_NULL;
  }
  for (int ch = 0; ch < num_channels; ch++) {
    LLVMValueRef get_args[] = {state_obj,
                               LLVMConstInt(i32_ty, (uint64_t)ch, 0)};
    vals[ch] =
        LLVMBuildCall2(builder, get_ty, get_fn, get_args, 2, "rb.sample.ch");
  }
  return DSP_MULTI(num_channels, vals);
}

static DspValue dsp_rubberband_bufplay_impl(Ast *ast, DspBuildCtx *dsp_ctx,
                                            JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder,
                                            int finer_engine) {
  Ast *args = ast->data.AST_APPLICATION.args;
  double num_channels_num = 0.0;
  if (!rb_try_eval_const_num(&args[0], dsp_ctx, &num_channels_num)) {
    fprintf(stderr,
            "Error: rbufplay expects a constant num_channels argument\n");
    return DSP_NULL;
  }

  int num_channels = (int)num_channels_num;
  if (num_channels <= 0) {
    fprintf(stderr, "Error: rbufplay num_channels must be > 0\n");
    return DSP_NULL;
  }

  DspValue buf_v = dsp_build_expr(args + 1, dsp_ctx, ctx, module, builder);
  DspValue pitch_v = dsp_build_expr(args + 2, dsp_ctx, ctx, module, builder);
  DspValue time_v = dsp_build_expr(args + 3, dsp_ctx, ctx, module, builder);
  DspValue start_pos_v =
      dsp_build_expr(args + 4, dsp_ctx, ctx, module, builder);
  DspValue trig_v = dsp_build_expr(args + 5, dsp_ctx, ctx, module, builder);

  LLVMValueRef buf = rb_dsp_value_lane(buf_v, 0);
  LLVMValueRef pitch =
      ensure_float(args[2].type, rb_dsp_value_lane(pitch_v, 0), builder);
  LLVMValueRef time_ratio =
      ensure_float(args[3].type, rb_dsp_value_lane(time_v, 0), builder);
  LLVMValueRef start_pos =
      ensure_float(args[4].type, rb_dsp_value_lane(start_pos_v, 0), builder);
  LLVMValueRef trig =
      ensure_float(args[5].type, rb_dsp_value_lane(trig_v, 0), builder);

  return build_rubberband_bufplay(buf, pitch, time_ratio, start_pos, trig,
                                  num_channels, finer_engine, dsp_ctx, module,
                                  builder);
}

DspValue dsp_rubberband_bufplay(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {
  return dsp_rubberband_bufplay_impl(ast, dsp_ctx, ctx, module, builder, 0);
}

DspValue dsp_rubberband_bufplay_finer(Ast *ast, DspBuildCtx *dsp_ctx,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {
  return dsp_rubberband_bufplay_impl(ast, dsp_ctx, ctx, module, builder, 1);
}
