
#include "scheduling.h"
#include "../lang/format_utils.h"
#include "ctx.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

// #define SCHED_DEBUG 1
#define SCHED_DEBUG 0

#if SCHED_DEBUG
#define SCHED_DBG(fmt, ...)                                                    \
  fprintf(stderr, COLOR_RED "[sched] " fmt "\n" STYLE_RESET_ALL, ##__VA_ARGS__)
#else
#define SCHED_DBG(fmt, ...) ((void)0)
#endif

static int timer_fd, wake_fd, epoll_fd;

static pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static void arm_timer(uint64_t target_tick) {
  uint64_t now = get_current_sample();
  struct itimerspec its = {0};
  if (target_tick <= now) {
    its.it_value.tv_nsec = 1; // fire immediately
    SCHED_DBG("arm_timer: tick=%llu <= now=%llu, firing immediately",
              (unsigned long long)target_tick, (unsigned long long)now);
  } else {
    double seconds = (double)(target_tick - now) / ctx_sample_rate();
    its.it_value.tv_sec = (time_t)seconds;
    its.it_value.tv_nsec =
        (long)((seconds - (double)its.it_value.tv_sec) * 1e9);
    if (its.it_value.tv_nsec == 0 && its.it_value.tv_sec == 0)
      its.it_value.tv_nsec = 1; // timerfd disarms on {0,0}
    SCHED_DBG("arm_timer: tick=%llu, now=%llu, delay=%.4fs",
              (unsigned long long)target_tick, (unsigned long long)now,
              seconds);
  }
  timerfd_settime(timer_fd, 0, &its, NULL);
}

void scheduler_wake() {
  uint64_t val = 1;
  write(wake_fd, &val, sizeof(val));
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

  // did this event become the new earliest?
  bool is_earliest = (queue->events[0].tick == target_time);
  pthread_mutex_unlock(&scheduler_mutex);

  if (is_earliest) {
    SCHED_DBG("push_event: new earliest tick=%llu, arming timer",
              (unsigned long long)target_time);
    arm_timer(target_time);
    scheduler_wake();
  } else {
    SCHED_DBG("push_event: tick=%llu (earliest=%llu, heap size=%zu)",
              (unsigned long long)target_time,
              (unsigned long long)queue->events[0].tick, queue->size);
  }
}
static _Thread_local uint64_t sched_now = 0;
static uint64_t cur_tick = 0;

uint64_t get_tl_tick() {
  return cur_tick == 0 ? get_current_sample() : cur_tick;
}
uint64_t get_sched_tick() { return sched_now; }

void scheduler_init_fds() {
  timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  wake_fd = eventfd(0, EFD_NONBLOCK);
  epoll_fd = epoll_create1(0);

  struct epoll_event ev = {.events = EPOLLIN};
  ev.data.fd = timer_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev);
  ev.data.fd = wake_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wake_fd, &ev);
}

// Batch of events to fire outside the lock
#define MAX_BATCH 64
static SchedulerEvent pending_batch[MAX_BATCH];

// Caller must hold scheduler_mutex. Returns count of events collected.
static int collect_due_events(uint64_t current_sample) {
  int count = 0;
  while (scheduler_queue.size > 0 &&
         scheduler_queue.events[0].tick <= current_sample &&
         count < MAX_BATCH) {
    pending_batch[count++] = pop_event(&scheduler_queue);
  }
  return count;
}

// Fire collected events. Must be called WITHOUT the mutex held,
// since callbacks may call push_event/schedule_event.
static void fire_events(int count) {
  for (int i = 0; i < count; i++) {
    SchedulerEvent event = pending_batch[i];
    if (event.userdata == NULL) {
      void (*cb)(uint64_t) = (void (*)(uint64_t))event.callback;
      cb(event.tick);
    } else {
      cur_tick = event.tick;
      event.callback(event.userdata, cur_tick);
    }
  }
}

uint64_t get_current_sample() { return atomic_load(&global_sample_position); }

void *scheduler_thread_fn(void *arg) {
  SCHED_DBG("thread started, epoll_fd=%d timer_fd=%d wake_fd=%d", epoll_fd,
            timer_fd, wake_fd);
  struct epoll_event events[2];
  for (;;) {
    int nfds = epoll_wait(epoll_fd, events, 2, -1); // blocks, zero CPU

    if (nfds < 0) {
      perror("[sched] epoll_wait");
      continue;
    }

    // drain the fds so they don't re-trigger
    for (int i = 0; i < nfds; i++) {
      uint64_t buf;
      read(events[i].data.fd, &buf, sizeof(buf));
      SCHED_DBG("woke: fd=%d (%s)", events[i].data.fd,
                events[i].data.fd == timer_fd ? "timer" : "eventfd");
    }

    sched_now = get_current_sample();
    uint64_t now = sched_now;

    // drain all due events before sleeping again — handles >MAX_BATCH
    // same-tick events and new events pushed by callbacks
    for (;;) {
      pthread_mutex_lock(&scheduler_mutex);
      SCHED_DBG("processing: now=%llu, heap size=%zu, earliest=%llu",
                (unsigned long long)now, scheduler_queue.size,
                scheduler_queue.size > 0
                    ? (unsigned long long)scheduler_queue.events[0].tick
                    : 0ULL);

      int count = collect_due_events(now);
      pthread_mutex_unlock(&scheduler_mutex);

      if (count == 0)
        break;

      SCHED_DBG("firing %d events", count);
      fire_events(count);
    }

    // re-arm timer to next event
    pthread_mutex_lock(&scheduler_mutex);
    if (scheduler_queue.size > 0) {
      SCHED_DBG("re-arming to next tick=%llu",
                (unsigned long long)scheduler_queue.events[0].tick);
      arm_timer(scheduler_queue.events[0].tick);
    } else {
      SCHED_DBG("heap empty, sleeping until next push");
    }
    pthread_mutex_unlock(&scheduler_mutex);
  }
  return NULL;
}

void *schedule_event(uint64_t now, double delay_seconds,
                     SchedulerCallback callback, void *userdata) {
  if (userdata == NULL)
    return userdata;
  int delay_samps = delay_seconds * ctx_sample_rate();
  SCHED_DBG("schedule_event: now=%llu delay_sec=%.6f sr=%d delay_samps=%d",
            (unsigned long long)now, delay_seconds, ctx_sample_rate(),
            delay_samps);
  push_event(callback, userdata, delay_samps, now);
  return userdata;
}

void defer_quant(double quant, DeferQuantCallback callback) {
  int sr = ctx_sample_rate();
  if (sr == 0) {
    sr = 48000;
  }

  uint64_t quant_samps = quant * sr;
  uint64_t now = get_current_sample();

  uint64_t offset_in_cycle = now % quant_samps;
  uint64_t remainder =
      (offset_in_cycle == 0) ? quant_samps : quant_samps - offset_in_cycle;

  // userdata=NULL signals fire_events to cast as DeferQuantCallback
  push_event((SchedulerCallback)callback, NULL, remainder, now);
}

int scheduler_event_loop() {
  init_heap(&scheduler_queue);
  scheduler_init_fds();

  pthread_t thread;
  if (pthread_create(&thread, NULL, scheduler_thread_fn, NULL) != 0) {
    fprintf(stderr, "Failed to create timer thread\n");
    return 1;
  }
  return 0;
}
