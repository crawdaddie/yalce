#include "./lib.h"
#include "./audio_graph.h"
#include "./ctx.h"
#include "./ext_lib.h"
#include "./node.h"
#include "./node_util.h"
#include "./osc.h"
#include "envelope.h"
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
  char *state = (char *)graph->nodes_state_memory + node->state_offset;
  return state;
}

void write_to_dac(int dac_layout, double *dac_buf, int _layout, double *buf,
                  int output_num, int nframes) {
  int layout = 1;

  if (output_num > 0) {

    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) += *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  } else {
    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) = *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
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

  if (head->perform) {

    head->perform(head, head + 1, NULL, frame_count, spf);

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

void user_ctx_callback(Ctx *ctx, int frame_count, double spf) {
  // reset_buf_ptr();
  //
  int consumed = process_msg_queue_pre(&ctx->msg_queue);
  if (ctx->head == NULL) {
    write_null_to_output_buf(ctx->output_buf, frame_count, LAYOUT);
  } else {
    perform_graph(ctx->head, frame_count, spf, ctx->output_buf, LAYOUT, 0);
  }
  process_msg_queue_post(&ctx->msg_queue, consumed);
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
      // Allocate state memory
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},

      .meta = "const",
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

void start_blob() {
  AudioGraph *graph = malloc(sizeof(AudioGraph));

  *graph = (AudioGraph){
      .nodes = malloc(16 * sizeof(Node)),
      .capacity = 16,
      .buffer_pool = malloc(sizeof(double) * (1 << 16)),
      .buffer_pool_capacity = 1 << 16,
      .nodes_state_memory = malloc(sizeof(char) * (1 << 6)),
      .state_memory_capacity = 1 << 6,
  };
  _graph = graph;
}

AudioGraph *end_blob() {
  AudioGraph *graph = _graph;
  print_graph(graph);

  graph->capacity = graph->node_count;
  graph->nodes = realloc(graph->nodes, (sizeof(Node) * graph->capacity));

  double *b = graph->buffer_pool;
  graph->buffer_pool_capacity = graph->buffer_pool_size;
  graph->buffer_pool = malloc(sizeof(double) * graph->buffer_pool_capacity);
  for (int i = 0; i < graph->buffer_pool_capacity; i++) {
    graph->buffer_pool[i] = b[i];
  }
  free(b);

  graph->state_memory_capacity = graph->state_memory_size;

  graph->nodes_state_memory =
      realloc(graph->nodes_state_memory, graph->state_memory_capacity);

  _graph = NULL;
  return graph;
}

AudioGraph *sin_ensemble() {
  AudioGraph *graph = malloc(sizeof(AudioGraph));

  *graph = (AudioGraph){
      .nodes = malloc(16 * sizeof(Node)),
      .capacity = 16,
      .buffer_pool = malloc(sizeof(double) * (1 << 12)),
      .buffer_pool_capacity = 1 << 12,
      .nodes_state_memory = malloc(sizeof(char) * (1 << 6)),
      .state_memory_capacity = 1 << 6,
  };
  _graph = graph;

  Node *f = inlet(150.);
  Node *g = inlet(1.);
  Node *s = sin_node(f);
  Node *env = asr_node(0.001, 0.8, 1.0, g);
  Node *m = mul2_node(env, s);

  graph->capacity = graph->node_count;
  graph->nodes = realloc(graph->nodes, (sizeof(Node) * graph->capacity));
  graph->buffer_pool_capacity = graph->buffer_pool_size;
  graph->buffer_pool = realloc(graph->buffer_pool,
                               (sizeof(double) * graph->buffer_pool_capacity));

  graph->state_memory_capacity = graph->state_memory_size;
  graph->nodes_state_memory =
      realloc(graph->nodes_state_memory, graph->state_memory_capacity);

  return _graph;
}

Node *instantiate_template(InValList *input_vals, AudioGraph *g) {

  // Allocate all required memory in one contiguous block
  char *mem =
      malloc(sizeof(Node) + sizeof(AudioGraph) + sizeof(Node) * g->capacity +
             sizeof(double) * g->buffer_pool_capacity +
             sizeof(char) * g->state_memory_capacity);

  // Set up the ensemble node at the start of memory
  Node *ensemble = (Node *)mem;
  mem += sizeof(Node);

  // Copy the AudioGraph structure next
  AudioGraph *graph_state = (AudioGraph *)mem;
  *graph_state = *g;
  mem += sizeof(AudioGraph);

  // Set up the nodes array
  graph_state->nodes = (Node *)mem;
  memcpy(graph_state->nodes, g->nodes, sizeof(Node) * g->capacity);
  mem += sizeof(Node) * g->capacity;

  // Set up the buffer pool
  graph_state->buffer_pool = (double *)mem;
  memcpy(graph_state->buffer_pool, g->buffer_pool,
         sizeof(double) * g->buffer_pool_capacity);
  // for (int i= 0; i < 4096; i++) {
  //   printf("preset buffer val %d: %f\n", i, graph_state->buffer_pool[i]);
  // }
  mem += sizeof(double) * g->buffer_pool_capacity;

  double *buf_mem = graph_state->buffer_pool;
  for (int i = 0; i < graph_state->node_count; i++) {
    graph_state->nodes[i].output.buf = buf_mem;
    buf_mem +=
        graph_state->nodes[i].output.layout * graph_state->nodes[i].output.size;
  }

  // Set up the state memory
  graph_state->nodes_state_memory = mem;
  memcpy(graph_state->nodes_state_memory, g->nodes_state_memory,
         g->state_memory_capacity);

  // print_graph(graph_state);
  // Assume the output node is the last node (the multiplier)
  Node *output_node = &graph_state->nodes[graph_state->node_count - 1];

  // Initialize the ensemble node
  *ensemble = (Node){.perform = (perform_func_t)perform_audio_graph,
                     .node_index = -1, // Special index for ensemble nodes
                     .num_inputs = 0,
                     .output = output_node->output,
                     .write_to_output = true,
                     .meta = "sin_ensemble",
                     .next = NULL};

  while (input_vals) {
    int idx = input_vals->pair.idx;
    double val = input_vals->pair.val;
    int inlet_node_idx = graph_state->inlets[idx];
    Node *inlet_node = graph_state->nodes + inlet_node_idx;
    // printf("set inlet node %d to %f\n", inlet_node_idx, val);
    for (int i = 0; i < inlet_node->output.layout * inlet_node->output.size;
         i++) {
      inlet_node->output.buf[i] = val;
    }

    input_vals = input_vals->next;
  }

  return ensemble;
}

Node *play_node_offset(int offset, Node *s) {
  // printf("play node %p at offset %d\n", s, offset);
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_ADD, offset, {.NODE_ADD = {.target = s}}});
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
      (scheduler_msg){NODE_SET_SCALAR,
                      offset,
                      {.NODE_SET_SCALAR = {
                           .target = target, .input = input, .value = 0.}}});
  free(p);
}

NodeRef play_node_offset_w_kill(int offset, double dur, int gate_in,
                                NodeRef s) {
  // printf("play node %p at offset %d %f %d\n", s, offset, dur, gate_in);
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  play_node_offset(offset, s);

  close_payload *cp = malloc(sizeof(close_payload));
  *cp = (close_payload){
      .target = s,
      .gate_input = gate_in,
  };
  schedule_event((SchedulerCallback)close_gate, cp, dur);
  return s;
}

NodeRef trigger_gate(int offset, double dur, int gate_in, NodeRef s) {
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           offset,
                           {.NODE_SET_SCALAR = {
                                .target = s, .input = gate_in, .value = 1.}}});

  close_payload *cp = malloc(sizeof(close_payload));
  *cp = (close_payload){
      .target = s,
      .gate_input = gate_in,
  };
  schedule_event((SchedulerCallback)close_gate, cp, dur);
  return s;
}

NodeRef play_node(NodeRef s) { return play_node_offset(get_frame_offset(), s); }

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
NodeRef load_soundfile(const char *path) {
  Node *sf = malloc(sizeof(Node) + sizeof(sf_meta));
  sf_meta *meta = (sf_meta *)((Node *)sf + 1);
  if (_read_file(path, &sf->output, &meta->sample_rate) != 0) {
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
                           {.NODE_SET_SCALAR = {node, input, value}}});
  return node;
}

NodeRef set_input_buf(int input, NodeRef buf, NodeRef node) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_INPUT,
                           get_frame_offset(),
                           {.NODE_SET_INPUT = {node, input, buf}}});
  return node;
}

NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node) {

  if ((char *)node->perform == (char *)perform_audio_graph) {
    AudioGraph *g = (AudioGraph *)((Node *)node + 1);
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

NodeRef set_input_scalar_offset(NodeRef node, int input, int frame_offset,
                                double value) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           frame_offset,
                           {.NODE_SET_SCALAR = {node, input, value}}});
  return node;
}

NodeRef set_input_trig(NodeRef node, int input) {
  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_SET_TRIG,
                                           get_frame_offset(),
                                           {.NODE_SET_TRIG = {node, input}}});
  return node;
}

NodeRef set_input_trig_offset(NodeRef node, int input, int frame_offset) {
  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_SET_TRIG,
                                           frame_offset,
                                           {.NODE_SET_TRIG = {node, input}}});
  return node;
}
