#include <stdio.h>
typedef struct ctx_t {
  int val;
  double outbus[2][512];
} ctx_t;

static ctx_t ctx = {.val = 1};

void init_user_ctx() {
  ctx.val = 2;
  ctx.outbus[0][0] = 1;
  ctx.outbus[1][256] = 1;
}
void user_ctx_callback(int frame_count, double seconds_per_frame) {
  printf("ctx val %d %d %f\n", ctx.val, frame_count, seconds_per_frame);
}
double read_channel_out(int chan, int frame) { return ctx.outbus[chan][frame]; }
