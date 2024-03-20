#include "ctx.h"
#include "common.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];

Ctx ctx;
static Node *tail = NULL;
void init_ctx() {

  ctx.main_vol = 0.125;

  ctx.dac_buffer.buf = *output_channel_pool;
  ctx.dac_buffer.size = BUF_SIZE;
  ctx.dac_buffer.layout = LAYOUT_CHANNELS;

  ctx.head = NULL;
  tail = ctx.head;
  ctx.msg_queue.num_msgs = 0;
  ctx.msg_queue.write_ptr = 0;
  ctx.msg_queue.read_ptr = 0;
}

Ctx *get_audio_ctx() { return &ctx; }
int ctx_sample_rate() { return ctx.sample_rate; }
Node *ctx_get_tail() { return tail; }

Node *ctx_add(Node *node) {
  Ctx *ctx = get_audio_ctx();
  // Node *tail = ctx_get_tail();
  if (ctx->head) {
    tail->next = node;
    tail = node;
  } else {
    ctx->head = node;
    tail = node;
  }
  return node;
}

UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {

  int consumed = process_msg_queue_pre(&ctx->msg_queue);
  // printf("consumed %d msgs\n", consumed);
  if (ctx->head == NULL) {
    return NULL;
  }
  perform_graph(ctx->head, nframes, seconds_per_frame, &ctx->dac_buffer, 0);
  // ctx->dac_bufer
  process_msg_queue_post(&ctx->msg_queue, consumed);
}

static inline int min(int a, int b) { return (a <= b) ? a : b; }

void write_to_output_buf(Signal *out, int nframes, double seconds_per_frame,
                         Signal *dac_sig, int output_num) {

  // write output to dac_buffer
  double *output = out->buf;
  int out_layout = out->layout;
  if (out_layout >= 2) {

    double *dest = dac_sig->buf;
    for (int f = 0; f < nframes; f++) {
      for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
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

      for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
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

Node *perform_graph(Node *head, int nframes, double seconds_per_frame,
                    Signal *dac_sig, int output_num) {
  if (!head) {
    return NULL;
  };

  if (head->killed) {
    Node *next = head->next;
    if (next) {
      return perform_graph(next, nframes, seconds_per_frame, dac_sig,
                           output_num);
    }
  }

  Signal *out = NULL;
  if (head->head) {
    Node *tail = perform_graph(head->head, nframes, seconds_per_frame, dac_sig,
                               output_num);
    out = tail->out;
  }

  if (head->perform) {
    head->perform(head, nframes, seconds_per_frame);
  }

  if (head->type == OUTPUT && out) {
    write_to_output_buf(out, nframes, seconds_per_frame, dac_sig, output_num);
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
