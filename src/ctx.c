#include "ctx.h" #include < stdlib.h>
#include "common.h"
#include "node.h"
#include <stdio.h>

static double output_channel_pool[OUTPUT_CHANNELS][BUF_SIZE * LAYOUT_CHANNELS];

Ctx ctx;
static Node *tail = NULL;
void init_ctx() {

  ctx.main_vol = 0.125;

  ctx.dac_buffer.data = *output_channel_pool;
  ctx.dac_buffer.size = BUF_SIZE;
  ctx.dac_buffer.layout = LAYOUT_CHANNELS;


  ctx.head = NULL;
  tail = ctx.head;
}

Ctx *get_audio_ctx() { return &ctx; }
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

}


UserCtxCb user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame) {
  if (ctx->head == NULL) {
    return NULL;
  }
  perform_graph(ctx->head, nframes, seconds_per_frame, &ctx->dac_buffer, 0);
  // ctx->dac_bufer
}

static inline int min(int a, int b) { return (a <= b) ? a : b; }
Node *perform_graph(Node *head, int nframes, double seconds_per_frame,
                    Signal *dac_buffer, int output_num) {
  if (head->killed) {
    Node *next = head->next;
    if (next) {
      return perform_graph(next, nframes, seconds_per_frame, dac_buffer,
                           output_num);
    }
  }

  if (!head) {
    return NULL;
  };

  if (head->perform && !(head->killed)) {
    head->perform(head, nframes, seconds_per_frame);
  }
  if (head->type == OUTPUT) {
    // write output to dac_buffer
    Signal output = head->out;
    int out_layout = output.layout;
    if (out_layout >= 2) {
      double *samps = output.data;
      double samp_val = *samps;
      double *dest = dac_buffer->data;
      for (int f = 0; f < nframes; f++) {
        for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
          if (output_num == 0) {
            *dest = *samps;
          } else {
            *dest += *samps;
          }
          if (ch <= out_layout) {
            samps++;
          }
          dest++;
        }
      }
    } else {
      double *samps = output.data;
      double *dest = dac_buffer->data;
      for (int f = 0; f < nframes; f++) {
        double samp_val = *samps;
        for (int ch = 0; ch < LAYOUT_CHANNELS; ch++) {
          if (output_num == 0) {
            *dest = samp_val;
          } else {
            *dest += samp_val;
          }
          dest++;
        }
        samps++;
      }
    }
    output_num++;
  }

  Node *next = head->next;

  if (next) {
    return perform_graph(next, nframes, seconds_per_frame, dac_buffer,
                         output_num); // keep going until you return tail
  };
  return head;
}
