#include "queue.h"

void *dequeue(queue_t *queue) {
  if (queue->tail == queue->head) {
    return NULL;
  }
  void *handle = queue->data[queue->tail];
  queue->data[queue->tail] = NULL;
  queue->tail = (queue->tail + 1) % queue->size;
  return handle;
}

int enqueue(queue_t *queue, void *handle) {
  if (((queue->head + 1) % queue->size) == queue->tail) {
    return 1;
  }
  queue->data[queue->head] = handle;
  queue->head = (queue->head + 1) % queue->size;
  return 0;
}

