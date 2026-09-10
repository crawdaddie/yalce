#include "audio_graph.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define YLC_AUDIO_GRAPH_RENDER_OP_CAPACITY 512u

static const ylc_node_allocator_t *ylc_current_node_allocator = NULL;

const ylc_node_allocator_t *ylc_node_allocator_get(void) {
  return ylc_current_node_allocator;
}

void ylc_node_allocator_set(const ylc_node_allocator_t *allocator) {
  ylc_current_node_allocator = allocator;
}

static uint32_t ylc_audio_graph_frame_capacity(Node *node) {
  if (!node || node->output.size <= 0) {
    return BUF_SIZE;
  }
  return (uint32_t)node->output.size;
}

static double ylc_audio_graph_spf(ylc_dummy_audio_graph_t *graph) {
  double rate = graph && graph->sample_rate
                    ? graph->sample_rate(graph->host_state)
                    : 48000.0;
  if (rate <= 0.0) {
    rate = 48000.0;
  }
  return 1.0 / rate;
}

static double ylc_audio_graph_read_output(Node *node, uint32_t frame,
                                          uint32_t channel) {
  if (!node || !node->output.buf || node->output.size <= 0 ||
      node->output.layout <= 0 || frame >= (uint32_t)node->output.size) {
    return 0.0;
  }

  const uint32_t layout = (uint32_t)node->output.layout;
  const uint32_t lane = channel < layout ? channel : layout - 1;
  return node->output.buf[(size_t)frame * (size_t)layout + lane];
}

static void ylc_audio_graph_clear_node_output(Node *node) {
  if (!node || !node->output.buf || node->output.size <= 0 ||
      node->output.layout <= 0) {
    return;
  }

  const size_t sample_count =
      (size_t)node->output.size * (size_t)node->output.layout;
  memset(node->output.buf, 0, sample_count * sizeof(double));
}

static void ylc_audio_graph_fill_node_output(Node *node, double value) {
  if (!node || !node->output.buf || node->output.size <= 0 ||
      node->output.layout <= 0) {
    return;
  }

  const size_t sample_count =
      (size_t)node->output.size * (size_t)node->output.layout;
  for (size_t i = 0; i < sample_count; ++i) {
    node->output.buf[i] = value;
  }
}

static void
ylc_dummy_audio_graph_clear_outputs(clap_audio_buffer_t *audio_outputs,
                                    uint32_t audio_outputs_count,
                                    uint32_t offset, uint32_t frames_count) {
  if (!audio_outputs || frames_count == 0) {
    return;
  }

  for (uint32_t port = 0; port < audio_outputs_count; ++port) {
    clap_audio_buffer_t *out = &audio_outputs[port];

    if (!out->data64) {
      // TODO: Add a float32 fallback if a host ignores the 64-bit preference.
      continue;
    }

    for (uint32_t ch = 0; ch < out->channel_count; ++ch) {
      double *dst = out->data64[ch];
      if (!dst) {
        continue;
      }
      for (uint32_t frame = 0; frame < frames_count; ++frame) {
        dst[offset + frame] = 0.0;
      }
      if (ch < 64) {
        out->constant_mask |= ((uint64_t)1u << ch);
      }
    }
  }
}

static void *ylc_audio_graph_node_state(Node *node) {
  if (!node) {
    return NULL;
  }
  return node->state_ptr ? node->state_ptr : (void *)(node + 1);
}

static bool ylc_audio_graph_render_plan_contains(ylc_dummy_audio_graph_t *graph,
                                                 Node *node) {
  if (!graph || !node) {
    return false;
  }

  for (uint32_t i = 0; i < graph->render_op_count; ++i) {
    if (graph->render_ops[i].node == node) {
      return true;
    }
  }
  return false;
}

static bool ylc_audio_graph_render_plan_add(ylc_dummy_audio_graph_t *graph,
                                            Node *node) {
  if (!graph || !node || node->trig_end ||
      ylc_audio_graph_render_plan_contains(graph, node)) {
    return true;
  }

  const int input_count =
      node->num_inputs < MAX_INPUTS ? node->num_inputs : MAX_INPUTS;
  for (int i = 0; i < input_count; ++i) {
    Node *input = (Node *)(uintptr_t)node->connections[i].source_node_index;
    if (input && !ylc_audio_graph_render_plan_add(graph, input)) {
      return false;
    }
  }

  if (!node->frame_perform && !node->write_to_output && !node->bus) {
    return true;
  }

  if (graph->render_op_count >= YLC_AUDIO_GRAPH_RENDER_OP_CAPACITY) {
    return false;
  }

  ylc_audio_graph_render_op_t *op =
      &graph->render_ops[graph->render_op_count++];
  memset(op, 0, sizeof(*op));
  op->node = node;
  op->state = ylc_audio_graph_node_state(node);
  for (int i = 0; i < input_count; ++i) {
    op->inputs[i] = (Node *)(uintptr_t)node->connections[i].source_node_index;
  }
  return true;
}

static void ylc_audio_graph_render_plan_build(ylc_dummy_audio_graph_t *graph) {
  if (!graph) {
    return;
  }

  graph->render_op_count = 0;
  for (Node *node = graph->head; node; node = node->next) {
    if (!ylc_audio_graph_render_plan_add(graph, node)) {
      break;
    }
  }
  graph->render_generation = graph->generation;
}

static void ylc_audio_graph_add_buffer(double *dst, uint32_t dst_channels,
                                       uint32_t dst_offset, Node *node,
                                       uint32_t frame, uint32_t frames_count) {
  if (!dst || !node || !node->output.buf || dst_channels == 0 ||
      frame >= frames_count) {
    return;
  }

  const uint32_t frame_capacity = ylc_audio_graph_frame_capacity(node);
  const uint32_t local_frame =
      frame_capacity > 0 ? frame % frame_capacity : frame;
  double *frame_dst = dst + ((size_t)(dst_offset + frame) * dst_channels);
  for (uint32_t ch = 0; ch < dst_channels; ++ch) {
    frame_dst[ch] += ylc_audio_graph_read_output(node, local_frame, ch);
  }
}

static void ylc_audio_graph_write_to_outputs(clap_audio_buffer_t *audio_outputs,
                                             uint32_t audio_outputs_count,
                                             uint32_t offset, uint32_t frame,
                                             uint32_t frames_count, Node *node,
                                             double amplitude) {
  if (!audio_outputs || !node || !node->write_to_output) {
    return;
  }

  for (uint32_t port = 0; port < audio_outputs_count; ++port) {
    clap_audio_buffer_t *out = &audio_outputs[port];
    if (!out->data64) {
      continue;
    }

    for (uint32_t ch = 0; ch < out->channel_count; ++ch) {
      double *dst = out->data64[ch];
      if (!dst) {
        continue;
      }
      const uint32_t frame_capacity = ylc_audio_graph_frame_capacity(node);
      const uint32_t local_frame =
          frame_capacity > 0 ? frame % frame_capacity : frame;
      dst[offset + frame] +=
          ylc_audio_graph_read_output(node, local_frame, ch) * amplitude;
    }
  }
  (void)frames_count;
}

static void ylc_audio_graph_render_op(ylc_dummy_audio_graph_t *graph,
                                      uint32_t op_index,
                                      clap_audio_buffer_t *audio_outputs,
                                      uint32_t audio_outputs_count,
                                      uint32_t offset, uint32_t frames_count,
                                      double spf, double amplitude) {
  if (!graph || op_index >= graph->render_op_count) {
    return;
  }

  ylc_audio_graph_render_op_t *op = &graph->render_ops[op_index];
  Node *node = op->node;
  if (!node || node->trig_end) {
    return;
  }

  int frame_offset = node->frame_offset;
  if (frame_offset < 0) {
    frame_offset = 0;
  } else if (frame_offset > (int)frames_count) {
    frame_offset = (int)frames_count;
  }

  for (uint32_t frame = (uint32_t)frame_offset; frame < frames_count; ++frame) {
    const uint32_t frame_capacity = ylc_audio_graph_frame_capacity(node);
    const uint32_t local_frame =
        frame_capacity > 0 ? frame % frame_capacity : frame;
    if (node->frame_perform) {
      node->frame_perform(node, op->state, op->inputs, (int)local_frame, spf);
    }

    if (node->bus && node->bus->output.buf && node->bus->output.layout > 0) {
      ylc_audio_graph_add_buffer(node->bus->output.buf,
                                 (uint32_t)node->bus->output.layout, 0, node,
                                 local_frame, frames_count);
    }

    ylc_audio_graph_write_to_outputs(audio_outputs, audio_outputs_count, offset,
                                     frame, frames_count, node, amplitude);

    if (node->trig_end) {
      break;
    }
  }

  node->frame_offset = 0;
}

static void ylc_dummy_audio_graph_on_process(
    void *state, const clap_audio_buffer_t *audio_inputs,
    uint32_t audio_inputs_count, clap_audio_buffer_t *audio_outputs,
    uint32_t audio_outputs_count, uint32_t offset, uint32_t frames_count,
    double gain) {
  (void)audio_inputs;
  (void)audio_inputs_count;

  if (!audio_outputs || frames_count == 0) {
    return;
  }

  ylc_dummy_audio_graph_t *graph = (ylc_dummy_audio_graph_t *)state;

  if (!graph) {
    ylc_dummy_audio_graph_clear_outputs(audio_outputs, audio_outputs_count,
                                        offset, frames_count);
    return;
  }

  if (!graph->head) {
    ylc_dummy_audio_graph_clear_outputs(audio_outputs, audio_outputs_count,
                                        offset, frames_count);
    return;
  }

  const double amplitude = 0.15 * gain;
  const double spf = ylc_audio_graph_spf(graph);

  ylc_dummy_audio_graph_clear_outputs(audio_outputs, audio_outputs_count,
                                      offset, frames_count);

  if (graph->render_generation != graph->generation) {
    ylc_audio_graph_render_plan_build(graph);
  }

  for (uint32_t port = 0; port < audio_outputs_count; ++port) {
    clap_audio_buffer_t *out = &audio_outputs[port];
    for (uint32_t ch = 0; out->data64 && ch < out->channel_count; ++ch) {
      if (ch < 64) {
        out->constant_mask &= ~((uint64_t)1u << ch);
      }
    }
  }

  for (uint32_t i = 0; i < graph->render_op_count; ++i) {
    ylc_audio_graph_render_op(graph, i, audio_outputs, audio_outputs_count,
                              offset, frames_count, spf, amplitude);
  }
}

static void ylc_dummy_audio_graph_on_note(void *state, uint32_t sample_offset,
                                          const ylc_runtime_note_t *note) {
  (void)state;
  (void)sample_offset;
  (void)note;
}

static void ylc_dummy_audio_graph_on_param(void *state, uint32_t sample_offset,
                                           clap_id param_id, double value) {
  (void)state;
  (void)sample_offset;
  (void)param_id;
  (void)value;
}

static void ylc_dummy_audio_graph_on_midi(void *state, uint32_t sample_offset,
                                          uint16_t port_index,
                                          const uint8_t data[3]) {
  (void)state;
  (void)sample_offset;
  (void)port_index;
  (void)data;
}

static void
ylc_dummy_audio_graph_on_transport(void *state, uint32_t sample_offset,
                                   const clap_event_transport_t *transport) {
  (void)state;
  (void)sample_offset;
  (void)transport;
}

static const ylc_program_vtable_t ylc_dummy_audio_graph_vtable = {
    .on_process = ylc_dummy_audio_graph_on_process,
    .on_note_on = ylc_dummy_audio_graph_on_note,
    .on_note_off = ylc_dummy_audio_graph_on_note,
    .on_param = ylc_dummy_audio_graph_on_param,
    .on_midi = ylc_dummy_audio_graph_on_midi,
    .on_transport = ylc_dummy_audio_graph_on_transport,
};

void ylc_dummy_audio_graph_init(ylc_dummy_audio_graph_t *graph,
                                void *host_state, uint32_t seed,
                                ylc_audio_graph_is_playing_fn is_playing,
                                ylc_audio_graph_sample_rate_fn sample_rate) {
  if (!graph) {
    return;
  }
  (void)seed;

  graph->program.vtable = &ylc_dummy_audio_graph_vtable;
  graph->program.state = graph;
  graph->host_state = host_state;
  graph->is_playing = is_playing;
  graph->sample_rate = sample_rate;
  graph->head = NULL;
  graph->tail = NULL;
  graph->alloc_head = NULL;
  graph->generation = 1;
  graph->render_generation = 0;
  graph->render_op_count = 0;
  memset(graph->render_ops, 0, sizeof(graph->render_ops));
}

ylc_program_t *ylc_dummy_audio_graph_program(ylc_dummy_audio_graph_t *graph) {
  return graph ? &graph->program : NULL;
}

Node *ylc_audio_graph_play_node(ylc_dummy_audio_graph_t *graph, Node *node) {
  if (!graph || !node) {
    return node;
  }

  node->write_to_output = true;
  for (Node *current = graph->head; current; current = current->next) {
    if (current == node) {
      graph->generation++;
      return node;
    }
  }

  node->next = NULL;
  if (!graph->head) {
    graph->head = node;
    graph->tail = node;
  } else {
    graph->tail->next = node;
    graph->tail = node;
  }
  graph->generation++;

  return node;
}

Node *ylc_audio_graph_reset_node(Node *node) {
  if (!node) {
    return node;
  }

  node->trig_end = false;
  if (node->state_ptr && node->state_size > 0) {
    memset(node->state_ptr, 0, (size_t)node->state_size);
  }
  ylc_audio_graph_clear_node_output(node);
  if (node->state_init) {
    node->state_init(node->state_ptr);
  }
  return node;
}

Node *ylc_audio_graph_create_scalar_node(double value) {
  const size_t buffer_bytes = (size_t)BUF_SIZE * sizeof(double);
  const size_t total = sizeof(Node) + buffer_bytes;

  const ylc_node_allocator_t *allocator = ylc_node_allocator_get();
  Node *node;
  if (allocator && allocator->alloc) {
    node = (Node *)allocator->alloc(total, allocator->user_data);
  } else {
    node = (Node *)calloc(1, total);
  }
  if (!node) {
    return NULL;
  }

  node->output = (Signal){
      .layout = 1,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node)),
  };
  node->meta = "scalar";
  ylc_audio_graph_fill_node_output(node, value);
  return node;
}

Node *ylc_audio_graph_set_scalar_node(Node *node, double value) {
  ylc_audio_graph_fill_node_output(node, value);
  return node;
}

Node *ylc_audio_graph_set_input_scalar(Node *node, int input, double value) {
  if (!node || input < 0 || input >= MAX_INPUTS) {
    return node;
  }

  Node *input_node =
      (Node *)(uintptr_t)node->connections[input].source_node_index;
  if (input_node) {
    ylc_audio_graph_set_scalar_node(input_node, value);
  }
  return node;
}

Node *ylc_audio_graph_play_voice(ylc_dummy_audio_graph_t *graph, Node *node) {
  if (!graph || !node) {
    return node;
  }

  ylc_audio_graph_reset_node(node);
  return ylc_audio_graph_play_node(graph, node);
}

void ylc_audio_graph_clear(ylc_dummy_audio_graph_t *graph) {
  if (!graph) {
    return;
  }

  for (Node *node = graph->head; node; node = node->next) {
    node->write_to_output = false;
  }
  graph->head = NULL;
  graph->tail = NULL;
  graph->generation++;
  graph->render_generation = 0;
  graph->render_op_count = 0;
  memset(graph->render_ops, 0, sizeof(graph->render_ops));

  ylc_audio_graph_free_all_nodes(graph);
}

double ylc_read_inlet_node(void *node_raw, int64_t frame) {
  if (frame < 0) {
    return 0.0;
  }
  return ylc_audio_graph_read_output((Node *)node_raw, (uint32_t)frame, 0);
}

void node_connect_input(int idx, NodeRef node, NodeRef input) {
  if (!node || idx < 0 || idx >= MAX_INPUTS) {
    return;
  }

  node->connections[idx].input_index = idx;
  node->connections[idx].source_node_index = (uint64_t)(uintptr_t)input;
  if (idx >= node->num_inputs) {
    node->num_inputs = idx + 1;
  }
}

static _Thread_local ylc_dummy_audio_graph_t *ylc_audio_graph_active = NULL;

void ylc_audio_graph_set_active_graph(ylc_dummy_audio_graph_t *graph) {
  ylc_audio_graph_active = graph;
}

ylc_dummy_audio_graph_t *ylc_audio_graph_get_active_graph(void) {
  return ylc_audio_graph_active;
}

static void *ylc_audio_graph_node_alloc(size_t size, void *user_data) {
  (void)user_data;
  void *ptr = calloc(1, size);
  if (!ptr) {
    return NULL;
  }

  ylc_dummy_audio_graph_t *graph = ylc_audio_graph_get_active_graph();
  if (graph) {
    Node *node = (Node *)ptr;
    node->alloc_next = graph->alloc_head;
    graph->alloc_head = node;
  }
  return ptr;
}

static void ylc_audio_graph_node_free(void *ptr, void *user_data) {
  (void)user_data;
  free(ptr);
}

static const ylc_node_allocator_t ylc_audio_graph_node_allocator = {
    .alloc = ylc_audio_graph_node_alloc,
    .free = ylc_audio_graph_node_free,
    .user_data = NULL,
};

void ylc_audio_graph_install_node_allocator(void) {
  ylc_node_allocator_set(&ylc_audio_graph_node_allocator);
}

void ylc_audio_graph_uninstall_node_allocator(void) {
  ylc_node_allocator_set(NULL);
}

void ylc_audio_graph_free_all_nodes(ylc_dummy_audio_graph_t *graph) {
  if (!graph) {
    return;
  }

  const ylc_node_allocator_t *allocator = ylc_node_allocator_get();
  Node *node = graph->alloc_head;
  while (node) {
    Node *next = node->alloc_next;
    if (allocator && allocator->free) {
      allocator->free(node, allocator->user_data);
    } else {
      free(node);
    }
    node = next;
  }
  graph->alloc_head = NULL;
}
