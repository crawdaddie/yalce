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
}

Ctx *get_audio_ctx() { return &ctx; }
int ctx_sample_rate() { return ctx.SR; }
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
  if (ctx->head == NULL) {
    return NULL;
  }
  perform_graph(ctx->head, nframes, seconds_per_frame, &ctx->dac_buffer, 0);
  // ctx->dac_bufer
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
