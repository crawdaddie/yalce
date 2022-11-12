#ifndef _QUEUE
#define _QUEUE
#include <stdlib.h>
typedef struct queue_t {
  size_t head;
  size_t tail;
  size_t size;
  void **data;
} t_queue;

void *dequeue(t_queue *queue);

int enqueue(t_queue *queue, void *handle);
#endif
