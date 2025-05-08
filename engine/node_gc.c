#include "./node_gc.h"
#include "ctx.h"
#include "group.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t node_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to remove and free a node from the linked list
void remove_and_free_node(ensemble_state *ctx, NodeRef prev, NodeRef to_free) {
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

  free(to_free);
}
void iter_gc(ensemble_state *ctx) {
  Node *current = ctx->head;

  Node *prev = NULL;

  while (current != NULL) {
    if (current->perform == (perform_func_t)perform_ensemble) {
      iter_gc(current->state_ptr);
    }

    Node *next = current->next; // Save next pointer before potentially freeing

    if (current->trig_end) {
      remove_and_free_node(ctx, prev, current);
      current = next;
      // Don't update prev since we removed a node
    } else {
      prev = current;
      current = next;
    }
  }
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
