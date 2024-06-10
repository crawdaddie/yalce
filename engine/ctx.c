#include "ctx.h"
#include "node.h"
#include <stdio.h>
#include <stdlib.h>

Ctx ctx;
void init_ctx() {}

//
// int consumed = process_msg_queue_pre(&ctx->msg_queue);
// // printf("consumed %d msgs\n", consumed);
// if (ctx->graph.head == NULL) {
//   write_null_to_output_buf(&ctx->dac_buffer, nframes);
//   return NULL;
// }
// perform_graph(ctx->graph.head, nframes, seconds_per_frame, &ctx->dac_buffer,
//               0);
// // ctx->dac_bufer
// process_msg_queue_post(&ctx->msg_queue, consumed);
//
//
static void write_null_to_output_buf(double *out, int nframes, int layout) {
  double *dest = out;
  for (int f = 0; f < nframes; f++) {
    for (int ch = 0; ch < layout; ch++) {
      *dest = 0.0;
      dest++;
    }
  }
}
void write_to_output(double *src, double *dest, int nframes, int output_num) {
  for (int f = 0; f < nframes; f++) {
    for (int ch = 0; ch < 2; ch++) {
      if (output_num > 0) {
        *dest += *(src + f);
      } else {
        *dest = *(src + f);
      }
      dest++;
    }
  }
}

static void offset_node_bufs(Node *node, int frame_offset) {

  if (frame_offset == 0) {
    return;
  }
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i] += frame_offset;
  }
}

static void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i] -= frame_offset;
  }
}

Node *perform_graph(Node *head, int nframes, double spf, double *dac_buf,
                    int output_num) {
  if (!head) {
    return NULL;
  }

  int frame_offset = 0;
  if (head->perform) {
    offset_node_bufs(head, frame_offset);

    head->perform(head->state, head->output_buf, head->num_ins, head->ins,
                  nframes, spf);
    unoffset_node_bufs(head, frame_offset);
  }

  if (head->type == OUTPUT) {
    write_to_output(head->output_buf, dac_buf + frame_offset,
                    nframes - frame_offset, output_num);
    output_num++;
  }

  Node *next = head->next;
  if (next) {
    // keep going until you return tail
    return perform_graph(next, nframes, spf, dac_buf, output_num);
  };
  return head;
}

void user_ctx_callback(Ctx *ctx, int frame_count, double spf) {
  if (ctx->head == NULL) {
    write_null_to_output_buf(ctx->output_buf, frame_count, LAYOUT);
  }
  perform_graph(ctx->head, frame_count, spf, ctx->output_buf, 0);
}

Node *audio_ctx_add(Node *node) {
  if (ctx.head == NULL) {
    ctx.head = node;
    ctx.tail = node;
    return node;
  }
  ctx.tail->next = node;
  ctx.tail = node;
  return node;
}

Node *add_to_dac(Node *node) { return NULL; }

Ctx *get_audio_ctx() { return &ctx; }
