#include "./ylc_lwt.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Promise queue structure
typedef struct promise_queue_node {
  Promise *promise;
  struct promise_queue_node *next;
} promise_queue_node;

typedef struct {
  promise_queue_node *head;
  promise_queue_node *tail;
  size_t size;
} PromiseQueue;

typedef void (*promise_callback_t)(void *data);
typedef void (*promise_error_callback_t)(void *error);

Promise *promise_create(void) {
  Promise *p = malloc(sizeof(Promise));
  if (!p)
    return NULL;

  p->status = PROMISE_PENDING;
  p->data = NULL;
  p->bindings = NULL;
  return p;
}

void promise_destroy(Promise *p) {
  if (!p)
    return;

  bound_cbs *current = p->bindings;
  while (current) {
    bound_cbs *next = current->next;
    free(current);
    current = next;
  }

  free(p);
}

PromiseQueue *queue_create(void) {
  PromiseQueue *q = malloc(sizeof(PromiseQueue));
  if (!q)
    return NULL;

  q->head = NULL;
  q->tail = NULL;
  q->size = 0;
  return q;
}

void queue_destroy(PromiseQueue *q) {
  if (!q)
    return;

  promise_queue_node *current = q->head;
  while (current) {
    promise_queue_node *next = current->next;
    free(current);
    current = next;
  }

  free(q);
}

int queue_enqueue(PromiseQueue *q, Promise *p) {
  if (!q || !p)
    return -1;

  promise_queue_node *node = malloc(sizeof(promise_queue_node));
  if (!node)
    return -1;

  node->promise = p;
  node->next = NULL;

  if (q->tail) {
    q->tail->next = node;
  } else {
    q->head = node;
  }

  q->tail = node;
  q->size++;
  return 0;
}

Promise *queue_dequeue(PromiseQueue *q) {
  if (!q || !q->head)
    return NULL;

  promise_queue_node *node = q->head;
  Promise *p = node->promise;

  q->head = node->next;
  if (!q->head) {
    q->tail = NULL;
  }

  q->size--;
  free(node);
  return p;
}

int queue_is_empty(PromiseQueue *q) { return !q || q->size == 0; }

size_t queue_size(PromiseQueue *q) { return q ? q->size : 0; }

int promise_bind_callback(Promise *p, void *callback) {
  if (!p || !callback)
    return -1;

  bound_cbs *new_binding = malloc(sizeof(bound_cbs));
  if (!new_binding)
    return -1;

  new_binding->cb = callback;
  new_binding->next = p->bindings;
  p->bindings = new_binding;

  if (p->status == PROMISE_FULFILLED) {
    promise_callback_t cb = (promise_callback_t)callback;
    cb(p->data);
  }

  return 0;
}

void promise_execute_callbacks(Promise *p) {
  if (!p || p->status != PROMISE_FULFILLED)
    return;

  bound_cbs *current = p->bindings;
  while (current) {
    if (current->cb) {
      promise_callback_t cb = (promise_callback_t)current->cb;
      cb(p->data);
    }
    current = current->next;
  }
}

int promise_resolve(Promise *p, void *data) {
  if (!p || p->status != PROMISE_PENDING)
    return -1;

  p->status = PROMISE_FULFILLED;
  p->data = data;

  promise_execute_callbacks(p);

  return 0;
}

int promise_reject(Promise *p, void *error) {
  if (!p || p->status != PROMISE_PENDING)
    return -1;

  p->status = PROMISE_ERROR;
  p->data = error;

  // Note: You might want separate error callbacks
  // For now, we'll just mark as error

  return 0;
}
int promise_fulfilled(Promise *p) { return p->status == PROMISE_FULFILLED; }
int promise_unfulfilled(Promise *p) { return p->status != PROMISE_FULFILLED; }
