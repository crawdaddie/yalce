#include "scheduling.h"
#include "lib.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define S_TO_NS 1000000000ULL
#define TIMER_INTERVAL_NS 100000ULL // 0.1 milliseconds

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SECONDS_TO_RUN 5

#define INITIAL_CAPACITY 16

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

void _schedule_event(EventHeap *heap, void (*callback)(void *, uint64_t),
                     void *userdata, double delay_seconds, uint64_t now) {

  if (heap->size >= heap->capacity) {
    heap->capacity *= 2;
    heap->events =
        realloc(heap->events, sizeof(ScheduledEvent) * heap->capacity);
  }

  uint64_t timestamp = now + (S_TO_NS * delay_seconds);

  // ScheduledEvent event = {callback, userdata, timestamp};
  // heap->events[heap->size] = event;
  // heapify_up(heap, heap->size);
  // heap->size++;
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

// Timer thread function
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
        Timer prev_timer = ev.timer;
        Timer t = {
            .now = ((double)(now - prev_timer._timer_start)) / S_TO_NS,
            .frame_offset = get_frame_offset(),
            // TODO: use specified event timestamp instead?
            ._abs_time = now,
            ._timer_start = prev_timer._timer_start,
        };
        ev.callback(t, ev.userdata);
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

// Example callback function
void example_cb(void *user_data, uint64_t t) {
  printf("Callback executed at %f\n", ((double)(t - start) / S_TO_NS));

  _schedule_event(queue, example_cb, NULL, 10, t);
}

int scheduler_event_loop() {
  queue = create_event_heap();

  pthread_t thread;

  now = get_time_ns();
  if (pthread_create(&thread, NULL, timer, queue) != 0) {
    fprintf(stderr, "Failed to create timer thread\n");
    return 1;
  }
  return 1;
}

void schedule_event(void (*callback)(void *, uint64_t), void *userdata,
                    double delay_seconds) {

  return _schedule_event(queue, callback, userdata, delay_seconds, now);
}

void __schedule_event(EventHeap *heap, void (*callback)(void *, uint64_t),
                      void *userdata, double delay_seconds, uint64_t now) {

  if (heap->size >= heap->capacity) {
    heap->capacity *= 2;
    heap->events =
        realloc(heap->events, sizeof(ScheduledEvent) * heap->capacity);
  }

  uint64_t timestamp = now + (S_TO_NS * delay_seconds);

  // ScheduledEvent event = {callback, userdata, timestamp};
  // heap->events[heap->size] = event;
  heapify_up(heap, heap->size);
  heap->size++;
}

void schedule(void (*callback)(double, int, void *), void *userdata,
              double delay_seconds, uint64_t thread_start) {

  if (queue->size >= queue->capacity) {
    queue->capacity *= 2;
    queue->events =
        realloc(queue->events, sizeof(ScheduledEvent) * queue->capacity);
  }

  uint64_t timestamp = now + (S_TO_NS * delay_seconds);

  ScheduledEvent event = {callback, userdata, timestamp, timer};
  queue->events[queue->size] = event;
  heapify_up(queue, queue->size);
  queue->size++;
}

Timer new_timer() {
  Timer t = (Timer){
      .now = 0.,
      .frame_offset = get_frame_offset(),
      ._abs_time = now,
      ._timer_start = now,
  };
  return t;
}

/*
typedef struct Timer {
  double now;
  int frame_offset;
  uint64_t _abs_time;
  uint64_t _timer_start;
} Timer;
*/

int timer_frame_offset(Timer t) { return t.frame_offset; }
double timer_phase(Timer t) { return t.now; }
