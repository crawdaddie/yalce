#include "./ctx.h"
#include "./node.h"
#include "audio_graph.h"
#include "audio_routing.h"
#include "ext_lib.h"
#include "group.h"
#include <stdlib.h>

Ctx ctx;

void init_ctx() {

  ctx.num_input_signals = num_signals;
  ctx.input_signals = malloc(sizeof(Signal) * num_signals);

  for (int i = 0; i < num_signals; i++) {
    int layout = signal_info[i].num_channels;

    ctx.input_signals[i].buf = calloc(1, sizeof(double) * BUF_SIZE * layout);
    ctx.input_signals[i].layout = layout;
    ctx.input_signals[i].size = BUF_SIZE;
  }
  ctx.main_vol = 0.25;
}

void audio_ctx_add(Node *node) {
  node->write_to_output = true;
  node_group_state *ctx = &get_audio_ctx()->graph;

  // Add to existing chain
  if (ctx->head == NULL) {
    ctx->head = node;
    ctx->tail = ctx->head;
  } else {
    // Find the end of the chain
    Node *current = ctx->head;
    while (current->next != NULL) {
      current = (Node *)current->next;
    }
    // Append to the end
    current->next = node;
  }
}

void audio_ctx_add_before(Node *target, Node *node) {
  node->write_to_output = true;
  node_group_state *ctx = &get_audio_ctx()->graph;

  if (ctx->head == NULL) {
    node->next = NULL;
    ctx->head = node;
    ctx->tail = node;
    return;
  }

  Node *prev = NULL;
  Node *current = ctx->head;
  while (current != NULL && current != target) {
    prev = current;
    current = current->next;
  }

  if (current == NULL) {
    current = ctx->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = node;
    node->next = NULL;
    return;
  }

  if (prev == NULL) {
    node->next = ctx->head;
    ctx->head = node;
    return;
  }

  node->next = prev->next;
  prev->next = node;
}

Node *add_to_dac(Node *node) {
  // return NULL;
  // node->type = OUTPUT;
  // node->write_to_dac = true;
  return node;
}

Ctx *get_audio_ctx() { return &ctx; }

int ctx_sample_rate() { return ctx.sample_rate; }
double ctx_spf() { return ctx.spf; }

double *ctx_main_out() { return ctx.output_buf; }
void set_main_vol(double vol) { ctx.main_vol = vol; }
