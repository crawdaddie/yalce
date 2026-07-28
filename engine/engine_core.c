#include "./engine_core.h"
#include "./audio_loop.h"
#include "./scheduling.h"
#include <math.h>
#include <sndfile.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  BINOP_ADD,
  BINOP_SUB,
  BINOP_MUL,
  BINOP_DIV,
  BINOP_MOD,
} BinOp;

typedef struct {
  BinOp op;
} BinOpState;

typedef struct {
  double phase;
  int shape;
} OscState;

typedef struct {
  double carrier_phase;
  double mod_phase;
} PmState;

static int clamp_frame_offset(int frame_offset, int frame_count) {
  if (frame_offset < 0) {
    return 0;
  }
  if (frame_offset > frame_count) {
    return frame_count;
  }
  return frame_offset;
}

static void zero_output(double *out, int nframes, int layout) {
  if (out && nframes > 0 && layout > 0) {
    memset(out, 0, (size_t)nframes * (size_t)layout * sizeof(double));
  }
}

static void *node_state(Node *node) {
  if (!node) {
    return NULL;
  }
  return node->state_ptr ? node->state_ptr : (void *)(node + 1);
}

static Node *node_alloc(int state_size, int layout, int size,
                        const char *meta) {
  if (layout <= 0) {
    layout = 1;
  }
  if (size <= 0) {
    size = BUF_SIZE;
  }

  size_t state_bytes = (size_t)((state_size + 7) & ~7);
  size_t buffer_bytes = (size_t)layout * (size_t)size * sizeof(double);
  Node *node = calloc(1, sizeof(Node) + state_bytes + buffer_bytes);
  if (!node) {
    return NULL;
  }

  node->state_size = (int)state_bytes;
  node->state_ptr = state_bytes ? (void *)((char *)node + sizeof(Node)) : NULL;
  node->output = (Signal){
      .layout = layout,
      .size = size,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->meta = (char *)meta;
  return node;
}

void audio_engine_mark_dirty(Ctx *ctx) {
  if (ctx) {
    ctx->graph.generation++;
  }
}

static int render_plan_reserve(AudioRenderPlan *plan, int needed) {
  if (needed <= plan->capacity) {
    return 1;
  }

  int capacity = plan->capacity ? plan->capacity : 16;
  while (capacity < needed) {
    capacity *= 2;
  }

  AudioRenderOp *ops = realloc(plan->ops, sizeof(AudioRenderOp) * capacity);
  if (!ops) {
    return 0;
  }

  plan->ops = ops;
  plan->capacity = capacity;
  return 1;
}

static int render_plan_contains(AudioRenderPlan *plan, Node *node) {
  for (int i = 0; i < plan->len; i++) {
    if (plan->ops[i].node == node) {
      return 1;
    }
  }
  return 0;
}

static int render_plan_add_node(AudioRenderPlan *plan, Node *node) {
  if (!node || node->trig_end || render_plan_contains(plan, node)) {
    return 1;
  }

  for (int i = 0; i < node->num_inputs && i < MAX_INPUTS; i++) {
    Node *input = (Node *)node->connections[i].source_node_index;
    if (input && !render_plan_add_node(plan, input)) {
      return 0;
    }
  }

  if (!node->frame_perform && !node->write_to_output && !node->bus) {
    return 1;
  }

  if (!render_plan_reserve(plan, plan->len + 1)) {
    return 0;
  }

  AudioRenderOp *op = &plan->ops[plan->len++];
  memset(op, 0, sizeof(*op));
  op->node = node;
  op->state = node_state(node);

  for (int i = 0; i < node->num_inputs && i < MAX_INPUTS; i++) {
    op->inputs[i] = (Node *)node->connections[i].source_node_index;
  }

  return 1;
}

static void render_plan_build(Ctx *ctx) {
  AudioRenderPlan *plan = &ctx->render_plan;
  plan->len = 0;

  for (Node *node = ctx->graph.head; node; node = node->next) {
    if (!render_plan_add_node(plan, node)) {
      break;
    }
  }

  plan->built_generation = ctx->graph.generation;
}

static void render_plan_execute_op(Ctx *ctx, AudioRenderOp *op, int frame_count,
                                   double spf) {
  Node *node = op->node;
  if (!node || node->trig_end) {
    return;
  }

  int frame_offset = clamp_frame_offset(node->frame_offset, frame_count);
  int rendered_frames = frame_count - frame_offset;
  if (rendered_frames <= 0) {
    node->frame_offset = 0;
    return;
  }

  if (node->frame_perform) {
    for (int frame = frame_offset; frame < frame_count; frame++) {
      node->frame_perform(node, op->state, op->inputs, frame, spf);
    }
  }

  if (node->bus) {
    write_to_dac(node->bus->output.layout,
                 node->bus->output.buf +
                     ((size_t)frame_offset * node->bus->output.layout),
                 node->output.layout,
                 node->output.buf +
                     ((size_t)frame_offset * node->output.layout),
                 1, rendered_frames);
  }

  if (node->write_to_output) {
    write_to_dac(
        LAYOUT, ctx->output_buf + ((size_t)frame_offset * (size_t)LAYOUT),
        node->output.layout,
        node->output.buf + ((size_t)frame_offset * node->output.layout), 1,
        rendered_frames);
  }

  node->frame_offset = 0;
}

static void render_plan_execute(Ctx *ctx, int frame_count, double spf) {
  AudioRenderPlan *plan = &ctx->render_plan;
  for (int i = 0; i < plan->len; i++) {
    render_plan_execute_op(ctx, &plan->ops[i], frame_count, spf);
  }
}

void write_to_dac(int dac_layout, double *dac_buf, int layout, double *buf,
                  int output_num, int nframes) {
  if (!dac_buf || !buf || dac_layout <= 0 || layout <= 0 || nframes <= 0) {
    return;
  }

  while (nframes--) {
    for (int c = 0; c < dac_layout; c++) {
      double sample = buf[c < layout ? c : 0];
      if (output_num > 0) {
        dac_buf[c] += sample;
      } else {
        dac_buf[c] = sample;
      }
    }
    buf += layout;
    dac_buf += dac_layout;
  }
}

double ylc_read_inlet_node(void *node_raw, int64_t frame) {
  Node *node = (Node *)node_raw;
  if (!node || !node->output.buf || frame < 0 || frame >= node->output.size) {
    return 0.0;
  }

  int layout = node->output.layout > 0 ? node->output.layout : 1;
  return node->output.buf[(size_t)frame * (size_t)layout];
}

void audio_engine_render(Ctx *ctx, int frame_count, double spf) {
  if (!ctx || frame_count <= 0) {
    return;
  }

  zero_output(ctx->output_buf, frame_count, LAYOUT);

  if (ctx->render_plan.built_generation != ctx->graph.generation) {
    render_plan_build(ctx);
  }

  render_plan_execute(ctx, frame_count, spf);
}

void user_ctx_callback(Ctx *ctx, uint64_t current_tick, int frame_count,
                       double spf) {
  int consumed =
      process_msg_queue_pre(current_tick, frame_count, &ctx->msg_queue);
  audio_engine_render(ctx, frame_count, spf);
  process_msg_queue_post(current_tick, frame_count, &ctx->msg_queue, consumed);
}

void node_connect_input(int idx, NodeRef node, NodeRef input) {
  if (!node || idx < 0 || idx >= MAX_INPUTS) {
    return;
  }
  node->connections[idx].input_index = idx;
  node->connections[idx].source_node_index = (uint64_t)input;
  if (idx >= node->num_inputs) {
    node->num_inputs = idx + 1;
  }
}

void plug_input_in_graph(int idx, NodeRef node, NodeRef input) {
  if (!node || idx < 0 || idx >= MAX_INPUTS) {
    return;
  }
  node_connect_input(idx, node, input);
  audio_engine_mark_dirty(get_audio_ctx());
}

NodeRef const_sig(double value) {
  Node *node = node_alloc(0, 1, BUF_SIZE, "const");
  if (!node) {
    return NULL;
  }
  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.buf[i] = value;
  }
  return node;
}

static double read_node(Node *node, int frame, int channel) {
  if (!node || !node->output.buf || node->output.layout <= 0) {
    return 0.0;
  }
  if (frame < 0) {
    frame = 0;
  }
  if (frame >= node->output.size) {
    frame = node->output.size - 1;
  }
  int ch = channel % node->output.layout;
  return node->output.buf[(frame * node->output.layout) + ch];
}

static void binop_frame(void *ptr, void *state_raw, void *inputs_raw, int frame,
                        double spf) {
  (void)spf;
  Node *node = ptr;
  BinOpState *state = state_raw;
  Node **inputs = inputs_raw;
  int layout = node->output.layout;

  for (int ch = 0; ch < layout; ch++) {
    double a = read_node(inputs[0], frame, ch);
    double b = read_node(inputs[1], frame, ch);
    double out = 0.0;

    switch (state->op) {
    case BINOP_ADD:
      out = a + b;
      break;
    case BINOP_SUB:
      out = a - b;
      break;
    case BINOP_MUL:
      out = a * b;
      break;
    case BINOP_DIV:
      out = b == 0.0 ? 0.0 : a / b;
      break;
    case BINOP_MOD:
      out = b == 0.0 ? 0.0 : fmod(a, b);
      break;
    }

    node->output.buf[(frame * layout) + ch] = out;
  }
}

static NodeRef binop_node(NodeRef input1, NodeRef input2, BinOp op,
                          const char *meta) {
  int layout = 1;
  if (input1 && input1->output.layout > layout) {
    layout = input1->output.layout;
  }
  if (input2 && input2->output.layout > layout) {
    layout = input2->output.layout;
  }

  Node *node = node_alloc(sizeof(BinOpState), layout, BUF_SIZE, meta);
  if (!node) {
    return NULL;
  }
  node->frame_perform = binop_frame;
  node->num_inputs = 2;
  ((BinOpState *)node_state(node))->op = op;
  node_connect_input(0, node, input1);
  node_connect_input(1, node, input2);
  return node;
}

NodeRef sum2_node(NodeRef input1, NodeRef input2) {
  return binop_node(input1, input2, BINOP_ADD, "sum");
}

NodeRef mul2_node(NodeRef input1, NodeRef input2) {
  return binop_node(input1, input2, BINOP_MUL, "mul");
}

NodeRef sub2_node(NodeRef input1, NodeRef input2) {
  return binop_node(input1, input2, BINOP_SUB, "sub");
}

NodeRef div2_node(NodeRef input1, NodeRef input2) {
  return binop_node(input1, input2, BINOP_DIV, "div");
}

NodeRef mod2_node(NodeRef input1, NodeRef input2) {
  return binop_node(input1, input2, BINOP_MOD, "mod");
}

static double osc_sample(OscState *state) {
  double phase = state->phase;
  switch (state->shape) {
  case 1:
    return phase < 0.5 ? 1.0 : -1.0;
  case 2:
    return (2.0 * phase) - 1.0;
  default:
    return sin(phase * 2.0 * PI);
  }
}

static void osc_frame(void *ptr, void *state_raw, void *inputs_raw, int frame,
                      double spf) {
  Node *node = ptr;
  OscState *state = state_raw;
  Node **inputs = inputs_raw;
  double freq = read_node(inputs[0], frame, 0);
  double sample = osc_sample(state);
  int layout = node->output.layout;

  for (int ch = 0; ch < layout; ch++) {
    node->output.buf[(frame * layout) + ch] = sample;
  }

  state->phase = fmod(state->phase + freq * spf, 1.0);
  if (state->phase < 0.0) {
    state->phase += 1.0;
  }
}

static NodeRef osc_node(NodeRef freq, int shape, const char *meta) {
  Node *node = node_alloc(sizeof(OscState), 1, BUF_SIZE, meta);
  if (!node) {
    return NULL;
  }
  node->frame_perform = osc_frame;
  node->num_inputs = 1;
  ((OscState *)node_state(node))->shape = shape;
  node_connect_input(0, node, freq);
  return node;
}

NodeRef sin_node(NodeRef freq) { return osc_node(freq, 0, "sin"); }
NodeRef sq_node(NodeRef freq) { return osc_node(freq, 1, "sq"); }
NodeRef saw_node(NodeRef freq) { return osc_node(freq, 2, "saw"); }

static void pm_frame(void *ptr, void *state_raw, void *inputs_raw, int frame,
                     double spf) {
  Node *node = ptr;
  PmState *state = state_raw;
  Node **inputs = inputs_raw;
  double freq = read_node(inputs[0], frame, 0);
  double mod_index = read_node(inputs[1], frame, 0);
  double mod_ratio = read_node(inputs[2], frame, 0);
  double mod = sin(state->mod_phase * 2.0 * PI) * mod_index;
  double sample = sin((state->carrier_phase + mod) * 2.0 * PI);

  node->output.buf[frame] = sample;
  state->carrier_phase = fmod(state->carrier_phase + freq * spf, 1.0);
  state->mod_phase = fmod(state->mod_phase + freq * mod_ratio * spf, 1.0);
}

NodeRef pm_node(NodeRef freq, NodeRef mod_index, NodeRef mod_ratio) {
  Node *node = node_alloc(sizeof(PmState), 1, BUF_SIZE, "pm");
  if (!node) {
    return NULL;
  }
  node->frame_perform = pm_frame;
  node->num_inputs = 3;
  node_connect_input(0, node, freq);
  node_connect_input(1, node, mod_index);
  node_connect_input(2, node, mod_ratio);
  return node;
}

NodeRef play_node_offset(uint64_t tick, NodeRef node) {
  if (!node) {
    return NULL;
  }
  push_msg(&ctx.msg_queue,
           (audio_instruction){NODE_ADD, tick, {.NODE_ADD = {.target = node}}});
  return node;
}

NodeRef play_node(NodeRef node) {
  return play_node_offset(get_tl_tick(), node);
}

NodeRef play_node_before(NodeRef target, NodeRef node) {
  push_msg(&ctx.msg_queue,
           (audio_instruction){
               NODE_ADD_BEFORE,
               get_tl_tick(),
               {.NODE_ADD_BEFORE = {.target = target, .node = node}}});
  return node;
}

typedef struct {
  NodeRef target;
  int gate_input;
} ClosePayload;

static void close_gate(ClosePayload *payload, uint64_t tick) {
  push_msg(
      &ctx.msg_queue,
      (audio_instruction){NODE_SET_SCALAR,
                          tick,
                          {.NODE_SET_SCALAR = {.target = payload->target,
                                               .input = payload->gate_input,
                                               .value = 0.0}}});
  free(payload);
}

NodeRef play_node_dur(uint64_t tick, double dur, int gate_in, NodeRef node) {
  play_node_offset(tick, node);
  ClosePayload *payload = malloc(sizeof(ClosePayload));
  if (payload) {
    *payload = (ClosePayload){.target = node, .gate_input = gate_in};
    schedule_event(tick, dur, (SchedulerCallback)close_gate, payload);
  }
  return node;
}

NodeRef play_node_offset_w_kill(uint64_t tick, double dur, int gate_in,
                                NodeRef node) {
  return play_node_dur(tick, dur, gate_in, node);
}

NodeRef set_input_scalar_offset(NodeRef node, int input, uint64_t tick,
                                double value) {
  push_msg(&ctx.msg_queue,
           (audio_instruction){NODE_SET_SCALAR,
                               tick,
                               {.NODE_SET_SCALAR = {.target = node,
                                                    .input = input,
                                                    .value = value}}});
  return node;
}

NodeRef set_input_scalar(NodeRef node, int input, double value) {
  return set_input_scalar_offset(node, input, get_tl_tick(), value);
}

NodeRef set_input_trig_offset(NodeRef node, int input, uint64_t tick) {
  push_msg(
      &ctx.msg_queue,
      (audio_instruction){NODE_SET_TRIG,
                          tick,
                          {.NODE_SET_TRIG = {.target = node, .input = input}}});
  return node;
}

NodeRef set_input_trig(NodeRef node, int input) {
  return set_input_trig_offset(node, input, get_tl_tick());
}

NodeRef set_input_buf(int input, NodeRef buf, NodeRef node) {
  push_msg(
      &ctx.msg_queue,
      (audio_instruction){
          NODE_SET_INPUT,
          get_tl_tick(),
          {.NODE_SET_INPUT = {.target = node, .input = input, .value = buf}}});
  return node;
}

NodeRef set_input_buf_immediate(int input, NodeRef buf, NodeRef node) {
  plug_input_in_graph(input, node, buf);
  return node;
}

NodeRef pipe_into(NodeRef filter, int idx, NodeRef node) {
  if (!filter || !node || idx < 0 || idx >= MAX_INPUTS) {
    return filter;
  }
  node_connect_input(idx, filter, node);
  node->write_to_output = false;
  return filter;
}

NodeRef play_into(NodeRef target, NodeRef node) {
  if (target && node && target->num_inputs > 0) {
    pipe_into(target, target->num_inputs - 1, node);
  }
  return node;
}

NodeRef play_into_offset(uint64_t tick, NodeRef target, NodeRef node) {
  if (target && node) {
    int input = target->num_inputs > 0 ? target->num_inputs - 1 : 0;
    node->write_to_output = false;
    push_msg(&ctx.msg_queue,
             (audio_instruction){NODE_PIPE_INPUT,
                                 tick,
                                 {.NODE_PIPE_INPUT = {.target = target,
                                                      .input = input,
                                                      .value = node}}});
  }
  return node;
}

NodeRef play_into_idx(NodeRef target, int idx, NodeRef node) {
  if (target && node) {
    pipe_into(target, idx, node);
  }
  return node;
}

double midi_to_freq(int midi_note) {
  return 440.0 * pow(2.0, ((double)midi_note - 69.0) / 12.0);
}

double dmidi_to_freq(double midi_note) {
  return 440.0 * pow(2.0, (midi_note - 69.0) / 12.0);
}

double semi_to_ratio(int semitones) {
  return pow(2.0, (double)semitones / 12.0);
}

SignalRef node_out(NodeRef node) { return node ? &node->output : NULL; }

double *sig_raw(SignalRef sig) { return sig ? sig->buf : NULL; }

int sig_size(SignalRef sig) { return sig ? sig->size : 0; }

int sig_layout(SignalRef sig) { return sig ? sig->layout : 0; }

NodeRef array_to_buf(struct arr a) {
  Node *node = node_alloc(0, 1, a.size, "array_buf");
  if (!node) {
    return NULL;
  }
  node->output.buf = a.data;
  return node;
}

NodeRef inlet(double default_val) { return const_sig(default_val); }

NodeRef multi_chan_inlet(int layout, double default_val) {
  Node *node = node_alloc(0, layout, BUF_SIZE, "inlet");
  if (!node) {
    return NULL;
  }
  for (int i = 0; i < layout * BUF_SIZE; i++) {
    node->output.buf[i] = default_val;
  }
  return node;
}

NodeRef hw_inlet(int idx) {
  Ctx *audio_ctx = get_audio_ctx();
  if (!audio_ctx || idx < 0 || idx >= audio_ctx->num_input_signals) {
    return NULL;
  }

  Node *node =
      node_alloc(0, audio_ctx->input_signals[idx].layout, BUF_SIZE, "hw_inlet");
  if (!node) {
    return NULL;
  }
  node->output = audio_ctx->input_signals[idx];
  return node;
}

NodeRef buf_ref(NodeRef buf) {
  if (!buf) {
    return NULL;
  }
  Node *node = node_alloc(0, buf->output.layout, buf->output.size, "buf_ref");
  if (!node) {
    return NULL;
  }
  node->output = buf->output;
  return node;
}

NodeRef render_to_buf(int frames, NodeRef node) {
  if (!node || frames <= 0) {
    return NULL;
  }

  Node *out = node_alloc(0, node->output.layout, frames, "render_buf");
  if (!out) {
    return NULL;
  }

  int rendered = 0;
  int old_write = node->write_to_output;
  node->write_to_output = true;
  Ctx render_ctx = *get_audio_ctx();
  double dac[BUF_SIZE * LAYOUT];
  render_ctx.output_buf = dac;
  render_ctx.graph = (node_group_state){
      .head = node,
      .tail = node,
      .generation = 1,
  };
  render_ctx.render_plan = (AudioRenderPlan){0};

  while (rendered < frames) {
    int nframes = frames - rendered;
    if (nframes > BUF_SIZE) {
      nframes = BUF_SIZE;
    }
    audio_engine_render(&render_ctx, nframes, ctx.spf);
    write_to_dac(node->output.layout,
                 out->output.buf +
                     ((size_t)rendered * (size_t)node->output.layout),
                 node->output.layout, node->output.buf, 0, nframes);
    rendered += nframes;
  }
  free(render_ctx.render_plan.ops);
  node->write_to_output = old_write;
  return out;
}

void node_replace(NodeRef a, NodeRef b) {
  if (!a || !b) {
    return;
  }
  Node saved = *a;
  *a = *b;
  a->next = saved.next;
  a->frame_offset = saved.frame_offset;
  a->write_to_output = saved.write_to_output;
  audio_engine_mark_dirty(get_audio_ctx());
}

NodeRef null_synth_node(void) { return NULL; }

NodeRef chain(NodeRef tail) { return tail; }

int _read_file(const char *filename, Signal *signal, int *sf_sample_rate) {

  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

  if (*filename == '~') {
    char *HOME = getenv("HOME");
    if (!HOME) {
      fprintf(stderr, "could not resolve ~");
      return 1;
    }

    char *mem = calloc(strlen(HOME) + strlen(filename), sizeof(char));
    sprintf(mem, "%s%s", HOME, filename + 1);
    filename = mem;
  }

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

// {	sf_count_t	frames ;
// 	int			samplerate ;
// 	int			channels ;
// 	int			format ;
// 	int			sections ;
// 	int			seekable ;
// } ;

typedef struct _SF_Open_Payload {
  SNDFILE *fd;
  uint64_t frames;
  int32_t samplerate;
  int32_t channels;
} _SF_Open_Payload;

typedef struct _SF_Open_Opt {
  char status;
  _SF_Open_Payload payload;
} _SF_Open_Opt;

_Static_assert(sizeof(_SF_Open_Payload) == 24,
               "SFInfo payload ABI must match YLC tuple payload size");
_Static_assert(offsetof(_SF_Open_Opt, payload) == 8,
               "Option payload must be aligned after the tag");
_Static_assert(
    sizeof(_SF_Open_Opt) == 32,
    "Option of SFInfo ABI must match { i8, { ptr, i64, i32, i32 } }");

static _String ylc_string_from_c_abi(uint64_t size_offset, const char *chars) {
  return (_String){
      .size = (int32_t)(size_offset & UINT32_C(0xffffffff)),
      .offset = (int32_t)(size_offset >> 32),
      .chars = chars,
  };
}

static _SF_Open_Opt sf_open_none(void) {
  return (_SF_Open_Opt){.status = 1, .payload = {0}};
}

static _SF_Open_Opt sf_open_some(SNDFILE *infile, const SF_INFO *sfinfo) {
  return (_SF_Open_Opt){
      .status = 0,
      .payload =
          {
              .fd = infile,
              .frames = (uint64_t)sfinfo->frames,
              .samplerate = sfinfo->samplerate,
              .channels = sfinfo->channels,
          },
  };
}

_SF_Open_Opt sf_open_opt(uint64_t path_size_offset, const char *path_chars) {
  _String path = ylc_string_from_c_abi(path_size_offset, path_chars);
  const char *filename = path.chars;

  SF_INFO sfinfo;
  SNDFILE *infile;
  char *full_path = NULL;

  if (!filename || path.size <= 0) {
    fprintf(stderr, "empty soundfile path\n");
    return sf_open_none();
  }

  if (*filename == '~') {
    char *HOME = getenv("HOME");
    if (!HOME) {
      fprintf(stderr, "could not resolve ~");
      return sf_open_none();
    }

    full_path = calloc(strlen(HOME) + strlen(filename), sizeof(char));
    if (!full_path) {
      return sf_open_none();
    }
    sprintf(full_path, "%s%s", HOME, filename + 1);
    filename = full_path;
  }

  if (!(infile =
            sf_open(filename, SFM_READ,
                    &sfinfo))) { /* Open failed so print an error message. */
    printf("Not able to open input file %s.\n", filename);
    /* Print the error message from libsndfile. */
    puts(sf_strerror(NULL));
    if (full_path) {
      free(full_path);
    }
    return sf_open_none();
  };

  if (full_path) {
    free(full_path);
  }
  return sf_open_some(infile, &sfinfo);
}
//
// NodeRef load_soundfile(_String path) {
//   Node *sf = malloc(sizeof(Node) + sizeof(sf_meta));
//   sf_meta *meta = (sf_meta *)((Node *)sf + 1);
//   if (_read_file(path.chars, &sf->output, &meta->sample_rate) != 0) {
//     return NULL;
//   }
//   // printf("created sf node %d %d (%d)\n", sf->output.layout,
//   sf->output.size,
//   // meta->sample_rate);
//
//   return sf;
// }

NodeRef load_soundfile(_String filename) {
  printf("load sf %s\n", filename.chars);
  return NULL;
}
