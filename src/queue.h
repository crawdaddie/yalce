#ifndef _QUEUE
#define _QUEUE
#include <stdlib.h>
typedef struct queue_t {
  size_t head;
  size_t tail;
  size_t size;
  void **data;
} queue_t;

void *dequeue(queue_t *queue);

int enqueue(queue_t *queue, void *handle);
#endif
