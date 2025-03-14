#include "scheduling.h"
#include "audio_loop.h"
// #include "lib.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int get_frame_offset() {
  struct timespec t;
  struct timespec btime = get_block_time();
  set_block_time(&t);
  int frame_offset = get_block_frame_offset(btime, t, 48000);
  // printf("frame offset %d\n", frame_offset);
  return frame_offset;
}

#define S_TO_NS 1000000000ULL
#define TIMER_INTERVAL_NS 100000ULL // 0.1 milliseconds

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SECONDS_TO_RUN 5

#define INITIAL_CAPACITY 16

typedef struct {
  void (*callback)(void *userdata, int frame_offset);
  void *userdata;
  uint64_t timestamp;
} ScheduledEvent;

typedef struct {
  ScheduledEvent *events;
  size_t capacity;
  size_t size;
} EventHeap;

EventHeap *create_event_heap() {
  EventHeap *heap = malloc(sizeof(EventHeap));
  heap->events = malloc(sizeof(ScheduledEvent) * INITIAL_CAPACITY);
  heap->capacity = INITIAL_CAPACITY;
  heap->size = 0;
  return heap;
}
void heap_swap_events(ScheduledEvent *a, ScheduledEvent *b) {
  ScheduledEvent temp = *a;
  *a = *b;
  *b = temp;
}
void heapify_up(EventHeap *heap, size_t index) {
  while (index > 0) {
    size_t parent = (index - 1) / 2;
    if (heap->events[index].timestamp < heap->events[parent].timestamp) {
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
        heap->events[left_child].timestamp < heap->events[smallest].timestamp) {
      smallest = left_child;
    }

    if (right_child < heap->size && heap->events[right_child].timestamp <
                                        heap->events[smallest].timestamp) {
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

static inline uint64_t get_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (uint64_t)ts.tv_sec * S_TO_NS + ts.tv_nsec;
}

uint64_t now;
uint64_t start;

void _schedule_event(EventHeap *heap, void (*callback)(void *, int),
                     double delay_seconds, void *userdata,
                     uint64_t start_time) {

  if (heap->size >= heap->capacity) {
    heap->capacity *= 2;
    heap->events =
        realloc(heap->events, sizeof(ScheduledEvent) * heap->capacity);
  }

  uint64_t timestamp = start_time + (S_TO_NS * delay_seconds);

  ScheduledEvent event = {callback, userdata, timestamp};
  heap->events[heap->size] = event;

  struct C *c = heap->events[heap->size].userdata;

  heapify_up(heap, heap->size);
  heap->size++;
}

ScheduledEvent pop_event(EventHeap *heap) {
  if (heap->size == 0) {
    // Handle empty heap case
    ScheduledEvent empty = {NULL, NULL, 0};
    return empty;
  }

  // Save the root element (event with earliest timestamp)
  ScheduledEvent earliest = heap->events[0];

  // Decrease the size of the heap
  heap->size--;

  if (heap->size > 0) {
    // Move the last element to the root
    heap->events[0] = heap->events[heap->size];

    // Restore the heap property
    heapify_down(heap, 0);
  }

  return earliest;
}

EventHeap *queue;
// static __thread int tl_offset;
static __thread int tl_offset;

int get_tl_frame_offset(void) { return tl_offset; }

struct seq_object {
  void *dur;
  int32_t frame_offset;
};
struct coroutine {
  int c;
  void *fn_ptr;
  void *next;
  struct seq_object *argv;
};

void *timer(void *arg) {
  EventHeap *queue = (EventHeap *)arg;
  start = get_time_ns();
  now = start;
  uint64_t nextTick = start;
  struct timespec sleep_time = {0, TIMER_INTERVAL_NS};

  while (1) {
    now = get_time_ns();

    if (now >= nextTick) {
      while (queue->size && queue->events[0].timestamp <= now) {
        ScheduledEvent ev = pop_event(queue);
        double now_d = ((double)(now - start) / S_TO_NS);
        tl_offset = get_frame_offset();

        // printf("popped event from queue: cb: %p coroutine: %p %d\n",
        //        ev.callback, ev.userdata, tl_offset);

        ev.callback(ev.userdata, tl_offset);
      }

      // Calculate next tick
      nextTick += TIMER_INTERVAL_NS;

      // If we've fallen behind, catch up
      if (nextTick <= now) {
        nextTick = now + TIMER_INTERVAL_NS;
      }
    }

    // Sleep for a short time to prevent busy-waiting
    nanosleep(&sleep_time, NULL);
  }

  return NULL;
}

int scheduler_event_loop() {
  queue = create_event_heap();

  pthread_t thread;

  now = get_time_ns();
  if (pthread_create(&thread, NULL, timer, queue) != 0) {
    fprintf(stderr, "Failed to create timer thread\n");
    return 1;
  }
  return 0;
}

void schedule_event(void (*callback)(void *, int), double delay_seconds,
                    void *userdata) {

  printf("sched event coroutine: %p callback: %p time: %f\n", userdata,
         callback, delay_seconds);

  if (delay_seconds == 0.0) {
    int frame_offset = get_frame_offset();
    return callback(userdata, frame_offset);
  }

  now = get_time_ns(); // Update 'now' before scheduling
  return _schedule_event(queue, callback, delay_seconds, userdata, now);
}

void schedule_event_quant(void (*callback)(void *, int), double quantization,
                          void *userdata) {

  now = get_time_ns(); // Update 'now' before scheduling
  double now_s = ((double)now) / S_TO_NS;

  return _schedule_event(queue, callback, fmod(now_s, quantization), userdata,
                         now);
}
