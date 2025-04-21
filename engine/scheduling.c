#include "./scheduling.h"
#include "ctx.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;

// Lock-free queue for sample-accurate events
typedef struct {
  void (*callback)(void *userdata, int frame_offset);
  void *userdata;
  uint64_t sample_time;
} AudioThreadEvent;

// Pre-allocated ring buffer for audio thread events
#define AUDIO_EVENT_BUFFER_SIZE 1024
AudioThreadEvent audio_events[AUDIO_EVENT_BUFFER_SIZE];
atomic_size_t audio_write_index = 0;
atomic_size_t audio_read_index = 0;

// Global sample position counter (updated by audio thread)
atomic_ullong global_sample_position = 0;

typedef struct {
  void (*callback)(void *userdata, uint64_t now);
  void *userdata;
  uint64_t tick;
} SchedulerEvent;

typedef struct {
  SchedulerEvent *events;
  size_t capacity;
  size_t size;
} EventHeap;

#define INITIAL_CAPACITY 64
void init_heap(EventHeap *heap) {
  heap->events = malloc(sizeof(SchedulerEvent) * INITIAL_CAPACITY);
  heap->capacity = INITIAL_CAPACITY;
  heap->size = 0;
}

void heap_swap_events(SchedulerEvent *a, SchedulerEvent *b) {
  SchedulerEvent temp = *a;
  *a = *b;
  *b = temp;
}

void heapify_up(EventHeap *heap, size_t index) {
  while (index > 0) {
    size_t parent = (index - 1) / 2;
    if (heap->events[index].tick < heap->events[parent].tick) {
      heap_swap_events(&heap->events[index], &heap->events[parent]);
      index = parent;
    } else {
      break;
    }
  }
}

void heapify_down(EventHeap *heap, size_t index) {
  while (1) {
    size_t left_child = 2 * index + 1;
    size_t right_child = 2 * index + 2;
    size_t smallest = index;

    if (left_child < heap->size &&
        heap->events[left_child].tick < heap->events[smallest].tick) {
      smallest = left_child;
    }

    if (right_child < heap->size &&
        heap->events[right_child].tick < heap->events[smallest].tick) {
      smallest = right_child;
    }

    if (smallest != index) {
      heap_swap_events(&heap->events[index], &heap->events[smallest]);
      index = smallest;
    } else {
      break;
    }
  }
}

EventHeap scheduler_queue = {};

SchedulerEvent pop_event(EventHeap *heap) {
  if (heap->size == 0) {
    SchedulerEvent empty = {NULL, NULL, 0};
    return empty;
  }

  SchedulerEvent earliest = heap->events[0];
  heap->size--;

  if (heap->size > 0) {
    heap->events[0] = heap->events[heap->size];
    heapify_down(heap, 0);
  }

  return earliest;
}

void push_event(void (*callback)(void *, uint64_t), void *userdata,
                uint64_t delay_in_samples, uint64_t base_time) {

  EventHeap *queue = &scheduler_queue;
  pthread_mutex_lock(&scheduler_mutex);

  if (queue->size >= queue->capacity) {
    queue->capacity *= 2;
    queue->events =
        realloc(queue->events, sizeof(SchedulerEvent) * queue->capacity);
  }

  // Calculate absolute timestamp for next event
  uint64_t target_time = base_time + delay_in_samples;

  SchedulerEvent event = {
      .callback = callback, .userdata = userdata, .tick = target_time};

  queue->events[queue->size] = event;

  heapify_up(queue, queue->size);
  queue->size++;

  pthread_mutex_unlock(&scheduler_mutex);
}
static uint64_t cur_tick = 0;
uint64_t get_tl_tick() {
  return cur_tick == 0 ? get_current_sample() : cur_tick;
}
void process_scheduler_events(uint64_t current_sample) {

  SchedulerEvent events[32];
  while (scheduler_queue.size > 0 &&
         scheduler_queue.events[0].tick <= current_sample) {

    SchedulerEvent event = pop_event(&scheduler_queue);
    if (event.userdata == NULL) {

      void (*cb)(uint64_t) = (void (*)(uint64_t))event.callback;
      cb(event.tick);
    } else {
      // fprintf(stderr, "event callback %llu\n", event.tick);
      void *udata = event.userdata;
      cur_tick = event.tick;
      // printf("ev callback %p %llu\n", udata, cur_tick);
      event.callback(udata, cur_tick);
    }
  }
}

uint64_t get_current_sample() { return atomic_load(&global_sample_position); }

void *scheduler_thread_fn(void *arg) {
  while (1) {
    move_overflow();

    uint64_t current_sample = get_current_sample();
    process_scheduler_events(current_sample);
    usleep(5000); // 5ms
  }
  return NULL;
}

int scheduler_event_loop() {
  init_heap(&scheduler_queue);

  pthread_t thread;

  if (pthread_create(&thread, NULL, scheduler_thread_fn, NULL) != 0) {
    fprintf(stderr, "Failed to create timer thread\n");
    return 1;
  }

  return 0;
}

void *schedule_event(uint64_t now, double delay_seconds,
                     SchedulerCallback callback, void *userdata) {
  // printf("schedule event %llu %f %p\n", now, delay_seconds, userdata);

  if (userdata == NULL) {
    return userdata;
  }

  // printf("scheduling event %f %p\n", delay_seconds, userdata);
  int delay_samps = delay_seconds * ctx_sample_rate();
  push_event(callback, userdata, delay_samps, now);

  // callback(userdata, now);
  //
  return userdata;
}

void defer_quant(double quant, DeferQuantCallback callback) {
  uint64_t quant_samps = quant * ctx_sample_rate();
  uint64_t current_sample = get_current_sample();

  uint64_t offset_in_cycle = current_sample % quant_samps;
  uint64_t remainder;

  if (offset_in_cycle == 0) {
    remainder = quant_samps;
  } else {
    remainder = quant_samps - offset_in_cycle;
  }

  EventHeap *queue = &scheduler_queue;
  pthread_mutex_lock(&scheduler_mutex);

  if (queue->size >= queue->capacity) {
    queue->capacity *= 2;
    queue->events =
        realloc(queue->events, sizeof(SchedulerEvent) * queue->capacity);
  }

  uint64_t target_time = current_sample + remainder;

  SchedulerEvent event = {
      .callback = (void *)callback, .userdata = NULL, .tick = target_time};

  queue->events[queue->size] = event;

  heapify_up(queue, queue->size);
  queue->size++;

  pthread_mutex_unlock(&scheduler_mutex);
}
