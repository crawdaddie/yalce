#ifndef _ENGINE_CTX_H
#define _ENGINE_CTX_H
#include "common.h"
#include "lib.h"

typedef struct {
  double output_buf[BUF_SIZE * LAYOUT];
  Node *head;
  Node *tail;
  int sample_rate;
} Ctx;
extern Ctx ctx;

Ctx *get_audio_ctx();
void init_ctx();

void user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

void write_to_output(double *src, double *dest, int nframes, int output_num);

Node *audio_ctx_add(Node *node);
Node *add_to_dac(Node *node);

Node *perform_graph(Node *head, int frame_count, double spf, double *output_buf,
                    int output_num);
#endif
