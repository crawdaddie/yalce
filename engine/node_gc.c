#include "./node_gc.h"
#include "ctx.h"
#include "group.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t node_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to remove and free a node from the linked list
void remove_and_free_node(node_group_state *ctx, NodeRef prev,
                          NodeRef to_free) {
  if (prev == NULL) {
    // Node to remove is the head
    ctx->head = to_free->next;
    if (ctx->head == NULL) {
      // List is now empty
      ctx->tail = NULL;
    }
  } else {
    // Node is in the middle or end of the list
    prev->next = to_free->next;
    if (to_free == ctx->tail) {
      // If we're removing the tail, update it
      ctx->tail = prev;
    }
  }

  if (strcmp(to_free->meta, "ensemble") == 0) {
    // printf("freeing %p\n", to_free);
    free(to_free->state_ptr);
    free(to_free);
  } else if (to_free->state_ptr) {
    // printf("freeing %p\n", to_free);
    free(to_free->state_ptr);
    free(to_free);
  } else {
    free(to_free);
  }
}
static int iter_gc_prev_lines = 0;
static int iter_gc_cur_lines = 0;
static int iter_gc_depth = 0;

// #define __DUMP_TABLE

void iter_gc(node_group_state *ctx) {
#ifdef __DUMP_TABLE
  if (iter_gc_depth == 0) {
    for (int i = 0; i < iter_gc_prev_lines; i++)
      fprintf(stderr, "\033[A\033[2K");
    iter_gc_cur_lines = 0;
    fprintf(stderr, "%-6s  %-18s  %s\n", "status", "ptr", "meta");
    fprintf(stderr, "------  ------------------  ----\n");
    iter_gc_cur_lines += 2;
  }
#endif

  iter_gc_depth++;

  Node *current = ctx->head;
  Node *prev = NULL;

  while (current != NULL) {
    if (current->perform == (perform_func_t)perform_ensemble)
      iter_gc(current->state_ptr);

    Node *next = current->next;

#ifdef __DUMP_TABLE
    fprintf(stderr, "%-6s  %p  %s\n", current->trig_end ? "KILL" : "live",
            (void *)current, current->meta ? current->meta : "?");
    iter_gc_cur_lines++;
#endif

    if (current->trig_end) {
      remove_and_free_node(ctx, prev, current);
      current = next;
    } else {
      prev = current;
      current = next;
    }
  }

#ifdef __DUMP_TABLE
  iter_gc_depth--;
  if (iter_gc_depth == 0) {
    iter_gc_prev_lines = iter_gc_cur_lines;
  }
#endif
}

// The garbage collection thread function
void *gc_thread_func(void *arg) {
  Ctx *ctx = (Ctx *)arg;

  while (1) {
    // Sleep for a bit to avoid constant CPU usage
    // Adjust this value based on your needs
    usleep(100000); // 100ms

    // pthread_mutex_lock(&node_mutex);
    iter_gc(&ctx->graph);

    // pthread_mutex_unlock(&node_mutex);
  }
}

void gc_loop(Ctx *ctx) {
  pthread_t gc_thread;
  pthread_create(&gc_thread, NULL, gc_thread_func, ctx);
  pthread_detach(gc_thread); // Detach so we don't have to join it later
}
