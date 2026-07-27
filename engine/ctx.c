#include "./ctx.h"
#include "./node.h"
#include "audio_routing.h"
#include <stdio.h>
#include <stdlib.h>

Ctx ctx = {};

void init_ctx() {
  ctx.output_buf_capacity = BUF_SIZE;
  ctx.output_buf =
      calloc((size_t)ctx.output_buf_capacity * LAYOUT, sizeof(double));

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
  if (!node) {
    return;
  }
  node->write_to_output = true;
  node_group_state *ctx = &get_audio_ctx()->graph;
  node->next = NULL;

  if (ctx->head == NULL) {
    ctx->head = node;
    ctx->tail = node;
  } else {
    ctx->tail->next = node;
    ctx->tail = node;
  }
  audio_ctx_mark_dirty();
}

void audio_ctx_add_before(Node *target, Node *node) {
  if (!node) {
    return;
  }
  node->write_to_output = true;
  node_group_state *ctx = &get_audio_ctx()->graph;

  if (ctx->head == NULL) {
    node->next = NULL;
    ctx->head = node;
    ctx->tail = node;
    audio_ctx_mark_dirty();
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
    ctx->tail = node;
    audio_ctx_mark_dirty();
    return;
  }

  if (prev == NULL) {
    node->next = ctx->head;
    ctx->head = node;
    audio_ctx_mark_dirty();
    return;
  }

  node->next = prev->next;
  prev->next = node;
  audio_ctx_mark_dirty();
}

Ctx *get_audio_ctx() { return &ctx; }

int ctx_sample_rate() { return ctx.sample_rate; }

double ctx_spf() { return ctx.spf; }

double *ctx_main_out() { return ctx.output_buf; }
void set_main_vol(double vol) { ctx.main_vol = vol; }

void audio_ctx_mark_dirty(void) { ctx.graph.generation++; }
