#include "./rubberband_ffi.h"

#include "../rubberband/rubberband/rubberband-c.h"

#include <cstdlib>
#include <cstring>

typedef struct YlcRubberbandState {
  RubberBandState stretcher;
  unsigned int channels;
  unsigned int scratch_frames;
  float *input_planar;
  float *output_planar;
  const float **input_channels;
  float **output_channels;
} YlcRubberbandState;

static RubberBandOptions ylc_rubberband_default_options(bool finer) {
  RubberBandOptions options = RubberBandOptionProcessRealTime |
                              RubberBandOptionThreadingNever |
                              RubberBandOptionPitchHighConsistency |
                              RubberBandOptionFormantPreserved |
                              RubberBandOptionChannelsTogether;
  if (finer) {
    options |= RubberBandOptionEngineFiner;
  }
  return options;
}

static void ylc_rubberband_release_scratch(YlcRubberbandState *state) {
  if (!state) {
    return;
  }
  free(state->input_planar);
  free(state->output_planar);
  free((void *)state->input_channels);
  free(state->output_channels);
  state->input_planar = NULL;
  state->output_planar = NULL;
  state->input_channels = NULL;
  state->output_channels = NULL;
  state->scratch_frames = 0;
}

static int ylc_rubberband_ensure_scratch(YlcRubberbandState *state,
                                         unsigned int frames) {
  if (!state) {
    return 0;
  }
  if (frames <= state->scratch_frames) {
    return 1;
  }

  ylc_rubberband_release_scratch(state);

  size_t sample_count = (size_t)frames * (size_t)state->channels;
  state->input_planar = (float *)calloc(sample_count, sizeof(float));
  state->output_planar = (float *)calloc(sample_count, sizeof(float));
  state->input_channels =
      (const float **)calloc(state->channels, sizeof(float *));
  state->output_channels = (float **)calloc(state->channels, sizeof(float *));

  if (!state->input_planar || !state->output_planar || !state->input_channels ||
      !state->output_channels) {
    ylc_rubberband_release_scratch(state);
    return 0;
  }

  for (unsigned int ch = 0; ch < state->channels; ++ch) {
    state->input_channels[ch] = state->input_planar + ((size_t)ch * frames);
    state->output_channels[ch] = state->output_planar + ((size_t)ch * frames);
  }

  state->scratch_frames = frames;
  return 1;
}

static void ylc_rubberband_interleaved_to_planar(YlcRubberbandState *state,
                                                 _DoubleArray input,
                                                 unsigned int frames) {
  for (unsigned int frame = 0; frame < frames; ++frame) {
    for (unsigned int ch = 0; ch < state->channels; ++ch) {
      size_t src_idx = ((size_t)frame * state->channels) + ch;
      size_t dst_idx = ((size_t)ch * frames) + frame;
      state->input_planar[dst_idx] = (float)input.data[src_idx];
    }
  }
}

static _DoubleArray ylc_rubberband_planar_to_interleaved(YlcRubberbandState *state,
                                                          unsigned int frames) {
  size_t sample_count = (size_t)frames * (size_t)state->channels;
  double *output = (double *)calloc(sample_count, sizeof(double));
  if (!output) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  for (unsigned int frame = 0; frame < frames; ++frame) {
    for (unsigned int ch = 0; ch < state->channels; ++ch) {
      size_t src_idx = ((size_t)ch * frames) + frame;
      size_t dst_idx = ((size_t)frame * state->channels) + ch;
      output[dst_idx] = (double)state->output_planar[src_idx];
    }
  }

  return (_DoubleArray){.size = (int32_t)sample_count, .data = output};
}

static void *ylc_rubberband_new_impl(int sample_rate, int channels,
                                     double time_ratio, double pitch_scale,
                                     bool finer) {
  if (sample_rate <= 0 || channels <= 0) {
    return NULL;
  }

  YlcRubberbandState *state =
      (YlcRubberbandState *)calloc(1, sizeof(YlcRubberbandState));
  if (!state) {
    return NULL;
  }

  state->channels = (unsigned int)channels;
  state->stretcher =
      rubberband_new((unsigned int)sample_rate, state->channels,
                     ylc_rubberband_default_options(finer), time_ratio,
                     pitch_scale);
  if (!state->stretcher) {
    free(state);
    return NULL;
  }

  return state;
}

extern "C" void *ylc_rubberband_new_realtime(int sample_rate, int channels,
                                             double time_ratio,
                                             double pitch_scale) {
  return ylc_rubberband_new_impl(sample_rate, channels, time_ratio, pitch_scale,
                                 false);
}

extern "C" void *ylc_rubberband_new_realtime_finer(int sample_rate,
                                                   int channels,
                                                   double time_ratio,
                                                   double pitch_scale) {
  return ylc_rubberband_new_impl(sample_rate, channels, time_ratio, pitch_scale,
                                 true);
}

extern "C" void ylc_rubberband_delete(void *handle) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (!state) {
    return;
  }
  if (state->stretcher) {
    rubberband_delete(state->stretcher);
  }
  ylc_rubberband_release_scratch(state);
  free(state);
}

extern "C" void ylc_rubberband_reset(void *handle) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (state && state->stretcher) {
    rubberband_reset(state->stretcher);
  }
}

extern "C" void ylc_rubberband_set_time_ratio(void *handle, double ratio) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (state && state->stretcher) {
    rubberband_set_time_ratio(state->stretcher, ratio);
  }
}

extern "C" void ylc_rubberband_set_pitch_scale(void *handle, double scale) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (state && state->stretcher) {
    rubberband_set_pitch_scale(state->stretcher, scale);
  }
}

extern "C" int ylc_rubberband_get_latency(void *handle) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (!state || !state->stretcher) {
    return 0;
  }
  return (int)rubberband_get_latency(state->stretcher);
}

extern "C" int ylc_rubberband_get_samples_required(void *handle) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (!state || !state->stretcher) {
    return 0;
  }
  return (int)rubberband_get_samples_required(state->stretcher);
}

extern "C" int ylc_rubberband_get_channel_count(void *handle) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (!state || !state->stretcher) {
    return 0;
  }
  return (int)rubberband_get_channel_count(state->stretcher);
}

extern "C" _DoubleArray ylc_rubberband_process(void *handle, _DoubleArray input,
                                               int final_block) {
  YlcRubberbandState *state = (YlcRubberbandState *)handle;
  if (!state || !state->stretcher || !input.data || input.size <= 0 ||
      state->channels == 0) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  if (((unsigned int)input.size % state->channels) != 0) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  unsigned int frames = (unsigned int)input.size / state->channels;
  if (!ylc_rubberband_ensure_scratch(state, frames)) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  ylc_rubberband_interleaved_to_planar(state, input, frames);
  rubberband_process(state->stretcher, state->input_channels, frames,
                     final_block ? 1 : 0);

  int available = rubberband_available(state->stretcher);
  if (available <= 0) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  if (!ylc_rubberband_ensure_scratch(state, (unsigned int)available)) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  unsigned int retrieved = rubberband_retrieve(state->stretcher,
                                               state->output_channels,
                                               (unsigned int)available);
  if (retrieved == 0) {
    return (_DoubleArray){.size = 0, .data = NULL};
  }

  return ylc_rubberband_planar_to_interleaved(state, retrieved);
}

extern "C" void ylc_rubberband_free_array(_DoubleArray arr) {
  free(arr.data);
}
