#include "user_ctx.h"
UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports) {

  UserCtx *ctx = malloc(sizeof(UserCtx));
  ctx->input_port = input_port;
  ctx->output_ports = output_ports;

  return ctx;
}

