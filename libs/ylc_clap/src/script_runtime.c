#include "script_runtime.h"

#include <stddef.h>

static void clear_channel(float *buffer, uint32_t offset,
                          uint32_t frames_count) {
  if (!buffer) {
    return;
  }

  for (uint32_t i = 0; i < frames_count; ++i) {
    buffer[offset + i] = 0.0f;
  }
}

static void copy_channel(float *dst, const float *src, uint32_t offset,
                         uint32_t frames_count, float gain) {
  if (!dst) {
    return;
  }

  if (!src) {
    clear_channel(dst, offset, frames_count);
    return;
  }

  if (dst == src && gain == 1.0f) {
    return;
  }

  for (uint32_t i = 0; i < frames_count; ++i) {
    dst[offset + i] = src[offset + i] * gain;
  }
}

static void bypass_on_process(void *state,
                              const clap_audio_buffer_t *audio_inputs,
                              uint32_t audio_inputs_count,
                              clap_audio_buffer_t *audio_outputs,
                              uint32_t audio_outputs_count, uint32_t offset,
                              uint32_t frames_count, double gain) {

  if (!audio_outputs || audio_outputs_count == 0) {
    return;
  }

  clap_audio_buffer_t *out = &audio_outputs[0];
  const clap_audio_buffer_t *in =
      audio_inputs_count > 0 ? &audio_inputs[0] : NULL;
  const uint32_t in_channels = in ? in->channel_count : 0;
  const float scalar = (float)gain;

  for (uint32_t ch = 0; ch < out->channel_count; ++ch) {
    const float *src =
        (in && in->data32 && ch < in_channels) ? in->data32[ch] : NULL;
    float *dst = out->data32 ? out->data32[ch] : NULL;
    copy_channel(dst, src, offset, frames_count, scalar);
  }
}

static void bypass_on_note(void *state, uint32_t sample_offset,
                           const ylc_runtime_note_t *note) {}

static void bypass_on_param(void *state, uint32_t sample_offset,
                            clap_id param_id, double value) {}

static void bypass_on_midi(void *state, uint32_t sample_offset,
                           uint16_t port_index, const uint8_t data[3]) {}

static void bypass_on_transport(void *state, uint32_t sample_offset,
                                const clap_event_transport_t *transport) {}

static const ylc_program_vtable_t bypass_vtable = {
    .on_process = bypass_on_process,
    .on_note_on = bypass_on_note,
    .on_note_off = bypass_on_note,
    .on_param = bypass_on_param,
    .on_midi = bypass_on_midi,
    .on_transport = bypass_on_transport,
};

void ylc_runtime_init_bypass_program(ylc_program_t *program) {
  if (!program) {
    return;
  }

  program->vtable = &bypass_vtable;
  program->state = NULL;
}

void ylc_runtime_process_span(const ylc_program_t *program,
                              const clap_process_t *process, uint32_t offset,
                              uint32_t frames_count, double gain) {
  if (!program || !program->vtable || !program->vtable->on_process ||
      !process || frames_count == 0) {
    return;
  }

  program->vtable->on_process(
      program->state, process->audio_inputs, process->audio_inputs_count,
      process->audio_outputs, process->audio_outputs_count, offset,
      frames_count, gain);
}

void ylc_runtime_note_on(const ylc_program_t *program, uint32_t sample_offset,
                         const clap_event_note_t *event) {
  if (!program || !program->vtable || !program->vtable->on_note_on || !event) {
    return;
  }

  const ylc_runtime_note_t note = {
      .note_id = event->note_id,
      .port_index = event->port_index,
      .channel = event->channel,
      .key = event->key,
      .velocity = event->velocity,
  };
  program->vtable->on_note_on(program->state, sample_offset, &note);
}

void ylc_runtime_note_off(const ylc_program_t *program, uint32_t sample_offset,
                          const clap_event_note_t *event) {
  if (!program || !program->vtable || !program->vtable->on_note_off || !event) {
    return;
  }

  const ylc_runtime_note_t note = {
      .note_id = event->note_id,
      .port_index = event->port_index,
      .channel = event->channel,
      .key = event->key,
      .velocity = event->velocity,
  };
  program->vtable->on_note_off(program->state, sample_offset, &note);
}

void ylc_runtime_param(const ylc_program_t *program, uint32_t sample_offset,
                       clap_id param_id, double value) {
  if (!program || !program->vtable || !program->vtable->on_param) {
    return;
  }

  program->vtable->on_param(program->state, sample_offset, param_id, value);
}

void ylc_runtime_midi(const ylc_program_t *program, uint32_t sample_offset,
                      const clap_event_midi_t *event) {
  if (!program || !program->vtable || !program->vtable->on_midi || !event) {
    return;
  }

  program->vtable->on_midi(program->state, sample_offset, event->port_index,
                           event->data);
}

void ylc_runtime_transport(const ylc_program_t *program, uint32_t sample_offset,
                           const clap_event_transport_t *event) {
  if (!program || !program->vtable || !program->vtable->on_transport ||
      !event) {
    return;
  }

  program->vtable->on_transport(program->state, sample_offset, event);
}
