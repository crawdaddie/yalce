#include "./node_gc.h"
#include "ctx.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Assuming you have a mutex to protect the node list
pthread_mutex_t node_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to remove and free a node from the linked list
void remove_and_free_node(Ctx *ctx, NodeRef prev, NodeRef to_free) {
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

  // free(to_free->out.buf);
  //
  // for (int i = 0; i < to_free->num_ins; i++) {
  //   SignalRef in = ((char *)to_free + to_free->input_offsets[i]);
  //   free(in->buf);
  // }
  // Free the node
  printf("freeing %p\n", to_free);
  free(to_free);
}

// The garbage collection thread function
void *gc_thread_func(void *arg) {
  Ctx *ctx = (Ctx *)arg;

  while (1) {
    // Sleep for a bit to avoid constant CPU usage
    // Adjust this value based on your needs
    usleep(100000); // 100ms

    // pthread_mutex_lock(&node_mutex);

    Node *current = ctx->head;
    Node *prev = NULL;

    while (current != NULL) {
      Node *next =
          current->next; // Save next pointer before potentially freeing

      if (current->can_free) {
        remove_and_free_node(ctx, prev, current);
        current = next;
        // Don't update prev since we removed a node
      } else {
        prev = current;
        current = next;
      }
    }

    // pthread_mutex_unlock(&node_mutex);
  }

  return NULL; // Thread will never actually return
}

// The main function to create and start the GC thread
void gc_loop(Ctx *ctx) {
  pthread_t gc_thread;
  pthread_create(&gc_thread, NULL, gc_thread_func, ctx);
  pthread_detach(gc_thread); // Detach so we don't have to join it later
}
