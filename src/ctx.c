#include "ctx.h"
#include "common.h"
#include "node.h"
#include "scheduling.h"
#include <stdio.h>
#include <stdlib.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];

Ctx ctx;
void init_ctx() {

  ctx.main_vol = 0.125;

  ctx.dac_buffer.buf = *output_channel_pool;
  ctx.dac_buffer.size = BUF_SIZE;
  ctx.dac_buffer.layout = LAYOUT_CHANNELS;

  ctx.msg_queue.num_msgs = 0;
  ctx.msg_queue.write_ptr = 0;
  ctx.msg_queue.read_ptr = 0;
  ctx.graph.head = NULL;
  ctx.graph.tail = NULL;
}

Ctx *get_audio_ctx() { return &ctx; }
int ctx_sample_rate() { return ctx.sample_rate; }
Node *ctx_get_tail() { return ctx.graph.tail; }

/*
 * adds a node after the tail of the audio ctx graph
 * */
Node *ctx_add(Node *node) {
  Ctx *ctx = get_audio_ctx();
  graph_add_tail(&(ctx->graph), node);
  return node;
}

void ctx_rm_node(Node *node) { graph_delete_node(&(ctx.graph), node); }

static inline int min(int a, int b) { return (a <= b) ? a : b; }

static void write_null_to_output_buf(Signal *out, int nframes) {
  double *dest = out->buf;
  for (int f = 0; f < nframes; f++) {
    for (int ch = 0; ch < out->layout; ch++) {
      *dest = 0.0;
      dest++;
    }
  }
}
void write_to_output_buf(Signal *out, int nframes, double seconds_per_frame,
                         Signal *dac_sig, int output_num) {

  // printf("write %p to output %p\n", out->buf, dac_sig->buf);
  // write output to dac_buffer
  double *output = out->buf;
  int out_layout = out->layout;
  if (out_layout >= 2) {
    double *dest = dac_sig->buf;
    for (int f = 0; f < nframes; f++) {
      for (int ch = 0; ch < dac_sig->layout; ch++) {
        if (output_num == 0) {
          *dest = *output;
        } else {
          *dest += *output;
        }
        if (ch <= out_layout) {
          output++;
        }
        dest++;
      }
    }
  } else {
    double *dest = dac_sig->buf;
    for (int f = 0; f < nframes; f++) {
      double samp_val = *output;

      for (int ch = 0; ch < dac_sig->layout; ch++) {
        if (output_num == 0) {
          *dest = samp_val;
        } else {
          *dest += samp_val;
        }
        dest++;
      }
      output++;
    }
  }
}

static void offset_node_bufs(Node *head, int frame_offset) {
  for (int i = 0; i < head->num_ins; i++) {
    head->ins[i]->buf += frame_offset;
  }
}

static void unoffset_node_bufs(Node *head, int frame_offset) {
  for (int i = 0; i < head->num_ins; i++) {
    head->ins[i]->buf -= frame_offset;
  }
}

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {

  int consumed = process_msg_queue_pre(&ctx->msg_queue);
  // printf("consumed %d msgs\n", consumed);
  if (ctx->graph.head == NULL) {
    write_null_to_output_buf(&ctx->dac_buffer, nframes);
    return NULL;
  }
  perform_graph(ctx->graph.head, nframes, seconds_per_frame, &ctx->dac_buffer,
                0);
  // ctx->dac_bufer
  process_msg_queue_post(&ctx->msg_queue, consumed);
}

Node *perform_graph(Node *head, int nframes, double seconds_per_frame,
                    Signal *dac_sig, int output_num) {
  if (!head) {
    write_null_to_output_buf(dac_sig, nframes);
    return NULL;
  };

  if (head->killed) {
    Node *next = head->next;
    if (next) {
      return perform_graph(next, nframes, seconds_per_frame, dac_sig,
                           output_num);
    }
  }

  int frame_offset = head->frame_offset;
  if (head->perform) {
    head->perform(head, nframes - frame_offset, seconds_per_frame);
  }

  if (head->type == OUTPUT) {
    Signal dac = (Signal){.buf = dac_sig->buf + frame_offset * dac_sig->layout,
                          .size = dac_sig->size,
                          .layout = dac_sig->layout};
    write_to_output_buf(head->out, nframes - frame_offset, seconds_per_frame,
                        &dac, output_num);
    output_num++;
  }

  Node *next = head->next;

  if (next) {
    return perform_graph(next, nframes, seconds_per_frame, dac_sig,
                         output_num); // keep going until you return tail
  };
  return head;
}

static void process_msg_pre(scheduler_msg msg) {

  switch (msg.type) {
  case NODE_ADD: {
    struct NODE_ADD payload = msg.body.NODE_ADD;
    int frame_offset = msg.frame_offset;
    offset_node_bufs(payload.target, frame_offset);
    payload.target->frame_offset = frame_offset;
    add_to_dac(payload.target);
    ctx_add(payload.target);
    break;
  }

  case NODE_SET_SCALAR: {
    struct NODE_SET_SCALAR payload = msg.body.NODE_SET_SCALAR;
    Node *node = payload.target;
    Signal *target_input = node->ins[payload.input];
    for (int i = msg.frame_offset; i < target_input->size; i++) {
      *(target_input->buf + i) = payload.value;
    }
    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.body.NODE_SET_TRIG;
    Node *node = payload.target;
    Signal *target_input = node->ins[payload.input];
    *(target_input->buf + msg.frame_offset) = 1.0;
    break;
  }
  default:
    break;
  }
}

static void process_msg_post(scheduler_msg msg) {
  switch (msg.type) {
  case NODE_ADD: {
    struct NODE_ADD payload = msg.body.NODE_ADD;
    int frame_offset = msg.frame_offset;
    unoffset_node_bufs(payload.target, frame_offset);
    payload.target->frame_offset = 0;
    break;
  }

  case NODE_SET_SCALAR: {
    struct NODE_SET_SCALAR payload = msg.body.NODE_SET_SCALAR;
    Node *node = payload.target;
    Signal *target_input = node->ins[payload.input];
    for (int i = 0; i < msg.frame_offset; i++) {
      *(target_input->buf + i) = payload.value;
    }
    break;
  }

  case NODE_SET_TRIG: {
    struct NODE_SET_TRIG payload = msg.body.NODE_SET_TRIG;
    Node *node = payload.target;
    Signal *target_input = node->ins[payload.input];
    *(target_input->buf + msg.frame_offset) = 0.0;
    break;
  }
  default:
    break;
  }
}

int process_msg_queue_pre(msg_queue *queue) {
  int read_ptr = queue->read_ptr;
  scheduler_msg *msg;
  int consumed = 0;
  while (read_ptr != queue->write_ptr) {
    // printf("msg queue pre %d %d\n", read_ptr, msg_queue->write_ptr);
    msg = queue->buffer + read_ptr;
    process_msg_pre(*msg);
    read_ptr = (read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
    consumed++;
  }
  return consumed;
}

void process_msg_queue_post(msg_queue *queue, int consumed) {
  scheduler_msg msg;
  while (consumed--) {
    // printf("msg queue post %d %d\n", msg_queue->read_ptr,
    // msg_queue->write_ptr);
    msg = pop_msg(queue);
    process_msg_post(msg);
  }
}

int get_block_offset() {

  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  return offset;
}
void add_node_msg(Node *target, int offset) {
  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_ADD,
                       offset,
                       {.NODE_ADD = (struct NODE_ADD){
                            target,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_scalar_at(Node *target, int offset, int input, double value) {

  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_trig_at(Node *target, int offset, int input) {
  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_scalar(Node *target, int input, double value) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_trig(Node *target, int input) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void push_msgs(int num_msgs, scheduler_msg *scheduler_msgs) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  for (int i = 0; i < num_msgs; i++) {
    printf("push msgs %d %p type: %d\n", offset, scheduler_msgs + i,
           (scheduler_msgs + i)->type);
    scheduler_msg msg = scheduler_msgs[i];
    msg.frame_offset = offset;
    push_msg(&ctx->msg_queue, msg);
  }
}
