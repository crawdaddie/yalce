#include "gc.h"
#include "ctx.h"
#include "scheduling.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Node *cleanup_graph(Graph *graph, Node *head) {
  if (!head) {
    return NULL;
  };

  Node *next = head->next;
  if (head->killed) {
    // head->perform(head, nframes, seconds_per_frame);
    if (head->is_group) {
      cleanup_graph((Graph *)head->state, ((Graph *)head->state)->head);
    }
    graph_delete_node(graph, head);
  }

  if (next) {
    return cleanup_graph(graph, next);
  };
  return NULL;
}
void audio_ctx_gc() {
  while (true) {
    Ctx *ctx = get_audio_ctx();
    // printf("cleanup thread\n");
    cleanup_graph(&ctx->graph, ctx->graph.head);
    msleep(250);
  }
}
void cleanup_job() {
  pthread_t cleanup_thread;
  if (pthread_create(&cleanup_thread, NULL, (void *)audio_ctx_gc, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
  }
}
