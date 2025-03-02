#ifndef _ENGINE_CTX_H
#define _ENGINE_CTX_H
#include "common.h"

typedef struct Node Node;
typedef struct {
  double output_buf[BUF_SIZE * LAYOUT];
  Node *head;
  Node *tail;
  int sample_rate;
  double spf;
} Ctx;

extern Ctx ctx;

Ctx *get_audio_ctx();
void init_ctx();

void user_ctx_callback(Ctx *ctx, int nframes, double seconds_per_frame);

void write_to_output(double *src, double *dest, int nframes, int output_num);

int ctx_sample_rate();
double ctx_spf();
#endif
