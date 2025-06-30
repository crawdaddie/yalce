#include "./lib.h"
#include "./audio_graph.h"
#include "./ctx.h"
#include "./ext_lib.h"
#include "./node.h"
#include "audio_loop.h"
#include "scheduling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sndfile.h>

void node_get_inputs(Node *node, AudioGraph *graph, Node *inputs[]) {
  int num_inputs = node->num_inputs;
  for (int i = 0; i < num_inputs; i++) {
    inputs[i] = graph->nodes + node->connections[i].source_node_index;
  }
}

char *node_get_state(Node *node, AudioGraph *graph) {
  if (graph == NULL) {
    // no graph context, node + state probably allocated together
    return (char *)node + sizeof(Node);
  }
  if (node->state_ptr) {
    return node->state_ptr;
  }
  char *state = (char *)graph->nodes_state_memory + node->state_offset;
  return state;
}

void write_to_dac(int dac_layout, double *dac_buf, int layout, double *buf,
                  int output_num, int nframes) {

  if (output_num > 0) {

    while (nframes--) {
      for (int c = 0; c < dac_layout; c++) {
        *(dac_buf + c) += *(buf + (c < layout ? c : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  } else {
    while (nframes--) {
      for (int c = 0; c < dac_layout; c++) {
        *(dac_buf + c) = *(buf + (c < layout ? c : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  }
}

void __node_get_inputs_raw(Node *node, Node *inputs[]) {
  int num_inputs = node->num_inputs;
  for (int i = 0; i < num_inputs; i++) {
    inputs[i] = (Node *)node->connections[i].source_node_index;
  }
}

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {

  if (!head) {
    // printf("Error: NULL head\n");
    return;
  }

  if (head->trig_end) {
    if (head->next) {
      return perform_graph(head->next, frame_count, spf, dac_buf, layout,
                           output_num);
    }
    return;
  }

  offset_node_bufs(head, head->frame_offset);

  Node *inputs[MAX_INPUTS];

  if (head->perform) {
    void *state = head + 1;

    if (head->state_ptr) {
      state = head->state_ptr;
    }

    __node_get_inputs_raw(head, inputs);
    head->perform(head, state, inputs, frame_count, spf);
    // if (head->bus) {
    //   NodeRef bus = head->bus;
    //   double *bus_buf = bus->output.buf;
    //   int layout = bus->output.layout;
    //   write_to_dac(layout, bus_buf + (head->frame_offset * layout),
    //                head->output.layout, head->output.buf, 1,
    //                frame_count - head->frame_offset);
    // } else
    if (head->write_to_output) {
      write_to_dac(layout, dac_buf + (head->frame_offset * layout),
                   head->output.layout, head->output.buf, output_num,
                   frame_count - head->frame_offset);
    }
  }
  unoffset_node_bufs(head, head->frame_offset);

  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}

static void write_null_to_output_buf(double *out, int nframes, int layout) {
  double *dest = out;
  while (nframes--) {
    for (int ch = 0; ch < layout; ch++) {
      *dest = 0.0;
      dest++;
    }
  }
}

void user_ctx_callback(Ctx *ctx, uint64_t current_tick, int frame_count,
                       double spf) {

  int consumed = process_msg_queue_pre(current_tick, &ctx->msg_queue);
  node_group_state graph = ctx->graph;

  if (graph.head == NULL) {
    write_null_to_output_buf(ctx->output_buf, frame_count, LAYOUT);
  } else {
    perform_graph(graph.head, frame_count, spf, ctx->output_buf, LAYOUT, 0);
  }
  process_msg_queue_post(current_tick, &ctx->msg_queue, consumed);
}

Node *audio_graph_inlet(AudioGraph *g, int inlet_idx) {
  Node *inlet = g->nodes + g->inlets[inlet_idx];
  return inlet;
}

Node *inlet(double default_val) {

  AudioGraph *graph = _graph;
  Node *f = allocate_node_in_graph(graph, 0);

  // Initialize node
  *f = (Node){
      .perform = NULL,
      .node_index = f->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      // Allocate output buffer
      // TODO: allocate const bufs as just .size = 1
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},

      .meta = "inlet",
  };

  for (int i = 0; i < BUF_SIZE; i++) {
    f->output.buf[i] = default_val;
    // printf("const node val %f\n", node->output.buf[i]);
  }

  f->meta = "inlet";
  _graph->inlets[_graph->num_inlets] = f->node_index;
  _graph->inlet_defaults[_graph->num_inlets] = default_val;
  _graph->num_inlets++;
  return f;
}

Node *hw_inlet(int idx) {
  Signal *sig = ctx.input_signals + idx;

  AudioGraph *graph = _graph;
  Node *f = allocate_node_in_graph(graph, 0);

  // Initialize node
  *f = (Node){
      .perform = NULL,
      .node_index = f->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      // Allocate output buffer
      // TODO: allocate const bufs as just .size = 1
      .output =
          (Signal){
              .layout = sig->layout,
              .size = sig->size,
              .buf = sig->buf,
          },

  };

  f->meta = "hw_inlet";
  return graph_embed(f);
}

Node *multi_chan_inlet(int layout, double default_val) {

  AudioGraph *graph = _graph;
  Node *f = allocate_node_in_graph(graph, 0);

  // Initialize node
  *f = (Node){
      .perform = NULL,
      .node_index = f->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      // Allocate output buffer
      .output =
          (Signal){.layout = layout,
                   .size = BUF_SIZE,
                   .buf = allocate_buffer_from_pool(graph, layout * BUF_SIZE)},

      .meta = "const",
  };

  for (int i = 0; i < layout * BUF_SIZE; i++) {
    f->output.buf[i] = default_val;
    // printf("const node val %f\n", node->output.buf[i]);
  }

  f->meta = "inlet";
  _graph->inlets[_graph->num_inlets] = f->node_index;
  _graph->inlet_defaults[_graph->num_inlets] = default_val;
  _graph->num_inlets++;
  return graph_embed(f);
}
NodeRef buf_ref(NodeRef buf) {

  AudioGraph *graph = _graph;
  Node *f = allocate_node_in_graph(graph, 0);

  // Initialize node
  *f = (Node){
      .perform = NULL,
      .node_index = f->node_index,
      .num_inputs = 0,
      // Allocate state memory
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .meta = "buf_ref",
  };

  f->output.buf = buf->output.buf;
  f->output.layout = buf->output.layout;
  f->output.size = buf->output.size;

  return graph_embed(f);
}

void start_blob() {
  AudioGraph *graph = malloc(sizeof(AudioGraph));

  *graph = (AudioGraph){
      .nodes = malloc(16 * sizeof(Node)),
      .capacity = 16,
      .buffer_pool = malloc(sizeof(double) * (1 << 16)),
      .buffer_pool_capacity = 1 << 16,
      .nodes_state_memory = malloc(sizeof(char) * (1 << 16)),
      .state_memory_capacity = 1 << 16,
  };
  _graph = graph;
}

AudioGraph *end_blob() {
  AudioGraph *graph = _graph;

  graph->capacity = graph->node_count;
  graph->nodes = realloc(graph->nodes, (sizeof(Node) * graph->capacity));

  double *b = graph->buffer_pool;
  graph->buffer_pool_capacity = graph->buffer_pool_size;
  graph->buffer_pool = calloc(graph->buffer_pool_capacity, sizeof(double));
  for (int i = 0; i < graph->buffer_pool_capacity; i++) {
    graph->buffer_pool[i] = b[i];
  }
  free(b);

  graph->state_memory_capacity = graph->state_memory_size;

  graph->nodes_state_memory =
      realloc(graph->nodes_state_memory, graph->state_memory_capacity);
  // print_graph(graph);

  _graph = NULL;
  return graph;
}

AudioGraph *compile_blob_template(void (*tpl_func)()) {
  start_blob();
  tpl_func();
  return end_blob();
}

Node *instantiate_template(InValList *input_vals, AudioGraph *g) {

  char *node_mem = malloc(sizeof(Node));

  int memsize = sizeof(AudioGraph) + sizeof(Node) * g->capacity +
                sizeof(double) * g->buffer_pool_capacity +
                sizeof(char) * g->state_memory_capacity;

  char *mem = malloc(memsize);

  Node *ensemble = (Node *)node_mem;

  AudioGraph *graph_state = (AudioGraph *)mem;

  *graph_state = *g;
  mem += sizeof(AudioGraph);

  graph_state->nodes = (Node *)mem;
  memcpy(graph_state->nodes, g->nodes, sizeof(Node) * g->capacity);
  mem += sizeof(Node) * g->capacity;

  graph_state->buffer_pool = (double *)mem;
  memcpy(graph_state->buffer_pool, g->buffer_pool,
         sizeof(double) * g->buffer_pool_capacity);
  mem += sizeof(double) * g->buffer_pool_capacity;

  double *buf_mem = graph_state->buffer_pool;
  for (int i = 0; i < graph_state->node_count; i++) {
    Node *n = graph_state->nodes + i;
    if (strcmp(n->meta, "buf_ref") == 0) {
      continue;
    } else if (strcmp(n->meta, "hw_inlet") == 0) {
      continue;
    } else {
      graph_state->nodes[i].output.buf = buf_mem;
      buf_mem += graph_state->nodes[i].output.layout *
                 graph_state->nodes[i].output.size;
    }
    if (n->state_ptr != NULL) {
      n->state_ptr = graph_state->nodes_state_memory + n->state_offset;
    }
  }

  graph_state->nodes_state_memory = mem;

  memcpy(graph_state->nodes_state_memory, g->nodes_state_memory,
         g->state_memory_capacity);

  Node *output_node = &graph_state->nodes[graph_state->node_count - 1];

  // printf("node count %d out %p %s\n", graph_state->node_count, output_node,
  //        output_node->meta);

  // printf("instantiate node with %d layout output %s\n", output_node->meta,
  //        output_node->output.layout);

  *ensemble = (Node){
      .perform = (perform_func_t)perform_audio_graph,
      .node_index = -1, // Special index for ensemble nodes
      .num_inputs = 0,
      .output = output_node->output,
      .write_to_output = true,
      .meta = "ensemble",
      .next = NULL,
      .state_ptr = graph_state,
  };

  uint32_t inputs_mask;

  while (input_vals) {

    int idx = input_vals->pair.idx;

    inputs_mask |= (1U << idx);
    double val = input_vals->pair.val;
    int inlet_node_idx = graph_state->inlets[idx];
    Node *inlet_node = graph_state->nodes + inlet_node_idx;

    for (int i = 0; i < inlet_node->output.layout * inlet_node->output.size;
         i++) {
      inlet_node->output.buf[i] = val;
    }

    input_vals = input_vals->next;
  }
  for (int idx = 0; idx < g->num_inlets; idx++) {

    // Skip this inlet if its bit is already set in the mask
    if (inputs_mask & (1U << idx)) {
      continue; // Skip to the next iteration
    }
    double val = g->inlet_defaults[idx];
    int inlet_node_idx = graph_state->inlets[idx];
    Node *inlet_node = graph_state->nodes + inlet_node_idx;
    for (int i = 0; i < inlet_node->output.layout * inlet_node->output.size;
         i++) {
      inlet_node->output.buf[i] = val;
    }
  }

  ensemble->next = NULL;
  return ensemble;
}

Node *play_node_offset(uint64_t tick, Node *s) {
  // printf("play node %p at offset %d\n", s, offset);
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_ADD, tick, {.NODE_ADD = {.target = s}}}, 512);

  return s;
}

typedef struct close_payload {
  NodeRef target;
  int gate_input;
} close_payload;
void close_gate(close_payload *p, int offset) {
  NodeRef target = p->target;
  int input = p->gate_input;

  push_msg(
      &ctx.msg_queue,
      (scheduler_msg){
          NODE_SET_SCALAR,
          offset,
          {.NODE_SET_SCALAR = {.target = target, .input = input, .value = 0.}}},
      512);
  free(p);
}

NodeRef play_node_dur(uint64_t tick, double dur, int gate_in, NodeRef s) {
  // printf("play node %p dur: %f\n", s, dur);
  play_node_offset(tick, s);

  push_msg(
      &ctx.msg_queue,
      (scheduler_msg){
          NODE_SET_SCALAR,
          tick + dur * ctx_sample_rate(),
          {.NODE_SET_SCALAR = {.target = s, .input = gate_in, .value = 0.}}},
      512);

  // close_payload *cp = malloc(sizeof(close_payload));
  // *cp = (close_payload){
  //     .target = s,
  //     .gate_input = gate_in,
  // };
  //
  // schedule_event(tick, dur, (SchedulerCallback)close_gate, cp);

  return s;
}

NodeRef trigger_gate(uint64_t tick, double dur, int gate_in, NodeRef s) {
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(
      &ctx.msg_queue,
      (scheduler_msg){
          NODE_SET_SCALAR,
          tick,
          {.NODE_SET_SCALAR = {.target = s, .input = gate_in, .value = 1.}}},
      512);

  close_payload *cp = malloc(sizeof(close_payload));
  *cp = (close_payload){
      .target = s,
      .gate_input = gate_in,
  };
  schedule_event(get_current_sample(), dur, (SchedulerCallback)close_gate, cp);
  return s;
}

NodeRef chain(NodeRef _t) {
  Node *group = group_node();
  node_group_state *gstate = group + 1;
  gstate->head = _chain_head;
  gstate->tail = _chain_tail;
  gstate->tail->write_to_output = true;

  _chain_head = NULL;
  _chain_tail = NULL;
  return group;
}

NodeRef play_node(NodeRef s) {
  if ((strcmp(s->meta, "ensemble") != 0) && (_chain_head != NULL)) {
    return play_node_offset(get_frame_offset(), chain(s));
  }

  return play_node_offset(get_frame_offset(), s);
}

int _read_file(const char *filename, Signal *signal, int *sf_sample_rate) {
  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

  if (!(infile =
            sf_open(filename, SFM_READ,
                    &sfinfo))) { /* Open failed so print an error message. */
    printf("Not able to open input file %s.\n", filename);
    /* Print the error message from libsndfile. */
    puts(sf_strerror(NULL));
    return 1;
  };

  if (sfinfo.channels > MAX_SF_CHANNELS) {
    printf("Not able to process more than %d channels\n", MAX_SF_CHANNELS);
    sf_close(infile);
    return 1;
  };

  size_t total_size = sfinfo.channels * sfinfo.frames;

  double *buf = calloc((int)total_size, sizeof(double));
  // double *buf = signal->buf;

  // reads channels in interleaved
  int read = sf_read_double(infile, buf, total_size);
  if (read != total_size) {
    printf("warning read failure, read %d != total size) %zu", read,
           total_size);
  }

  sf_close(infile);
  signal->size = sfinfo.frames;
  signal->layout = sfinfo.channels;
  signal->buf = buf;
  *sf_sample_rate = sfinfo.samplerate;
  fprintf(stderr,
          "read %d frames from '%s' buf %p [channels: %d samplerate: %d]\n",
          read, filename, buf, sfinfo.channels, sfinfo.samplerate);
  return 0;
};

typedef struct {
  int sample_rate;
} sf_meta;

NodeRef load_soundfile(_YLC_String path) {
  Node *sf = malloc(sizeof(Node) + sizeof(sf_meta));
  sf_meta *meta = (sf_meta *)((Node *)sf + 1);
  if (_read_file(path.chars, &sf->output, &meta->sample_rate) != 0) {
    return NULL;
  }
  // printf("created sf node %d %d (%d)\n", sf->output.layout, sf->output.size,
  // meta->sample_rate);

  return sf;
}

// void set_input_scalar_offset(NodeRef target, int input, int offset, double
// val) {
//   push_msg(
//       &ctx.msg_queue,
//       (scheduler_msg){NODE_SET_SCALAR,
//                       offset,
//                       {.NODE_SET_SCALAR = {
//                            .target = target, .input = input, .value =
//                            val}}});
// }
//
NodeRef set_input_scalar(NodeRef node, int input, double value) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           get_frame_offset(),
                           {.NODE_SET_SCALAR = {node, input, value}}},
           512);
  return node;
}

NodeRef set_input_buf(int input, NodeRef buf, NodeRef node) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_INPUT,
                           get_frame_offset(),
                           {.NODE_SET_INPUT = {node, input, buf}}},
           512);
  return node;
}

NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node) {

  if ((char *)node->perform == (char *)perform_audio_graph) {

    AudioGraph *g = (AudioGraph *)((Node *)node + 1);
    if (node->state_ptr) {
      g = node->state_ptr;
    }

    Node *inlet_node = g->nodes + g->inlets[input];
    Signal inlet_data = inlet_node->output;
    inlet_node->output.layout = buf->output.layout;
    inlet_node->output.size = buf->output.size;
    inlet_node->output.buf = buf->output.buf;
    // printf("setting input data\n");
    // for (int i= 0; i < inlet_node->output.size * inlet_node->output.layout;
    // i++) { printf("buf data inlet: %f\n", inlet_node->output.buf[i]);
    // }
  }
  return node;
}

NodeRef set_input_scalar_offset(NodeRef node, int input, uint64_t tick,
                                double value) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           tick,
                           {.NODE_SET_SCALAR = {node, input, value}}},
           512);
  return node;
}

NodeRef set_input_trig(NodeRef node, int input) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){
               NODE_SET_TRIG, get_tl_tick(), {.NODE_SET_TRIG = {node, input}}},
           512);
  return node;
}

NodeRef set_input_trig_offset(NodeRef node, int input, uint64_t tick) {
  push_msg(
      &ctx.msg_queue,
      (scheduler_msg){NODE_SET_TRIG, tick, {.NODE_SET_TRIG = {node, input}}},
      512);
  return node;
}

double midi_to_freq(int midi_note) {
  // A4 (MIDI note 69) has a frequency of 440 Hz
  // Each semitone is a factor of 2^(1/12)
  return 440.0 * pow(2.0, (midi_note - 69) / 12.0);
}

Signal *node_out(NodeRef node) { return &(node->output); }
double *sig_raw(Signal *sig) { return sig->buf; }
int sig_size(Signal *sig) { return sig->size; }
int sig_layout(Signal *sig) { return sig->layout; }

NodeRef render_to_buf(int frames, NodeRef node) {
  int layout = node->output.layout;

  Node *out = malloc(sizeof(Node) + (sizeof(double) * frames * layout));
  out->output.layout = layout;
  out->output.size = frames;
  out->output.buf = (double *)((Node *)out + 1);
  double *buf = out->output.buf;
  node->write_to_output = true;
  double *b = buf;
  int rendered_frames = 0;
  for (int i = 0; i < (frames / BUF_SIZE); i++) {
    perform_graph(node, BUF_SIZE, ctx.spf, b, layout, 0);
    b += (BUF_SIZE * layout);
    rendered_frames += BUF_SIZE;
  }

  perform_graph(node, frames - rendered_frames, ctx.spf, b, layout, 0);

  node->write_to_output = false;
  return out;
}

NodeRef __array_to_buf(int layout, int size, double *data) {

  Node *out = malloc(sizeof(Node));
  out->output.layout = layout;
  out->output.size = size;
  out->output.buf = data;
  out->write_to_output = false;

  return out;
}

NodeRef array_to_buf(struct arr a) { return __array_to_buf(1, a.size, a.data); }

void node_replace(NodeRef a, NodeRef b) {
  if (!((strcmp(a->meta, "ensemble") == 0) &&
        (strcmp(b->meta, "ensemble") == 0))) {
    // TODO: replace other types of nodes too
    return;
  }

  Node a_specs = *a;
  *a = *b;
  a->next = a_specs.next;
  a->frame_offset = a_specs.frame_offset;
  a->write_to_output = a_specs.write_to_output;
  if (a_specs.state_ptr) {
    free(a_specs.state_ptr);
  }
}

// NodeRef node_in(int idx, NodeRef a) {
//   return a->connections[idx]
// }
