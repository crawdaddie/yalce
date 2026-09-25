
#include "scheduling.h"
#include "../lang/format_utils.h"
#include "ctx.h"
#ifdef __APPLE__
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#else
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#endif
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// #define SCHED_DEBUG 1
#define SCHED_DEBUG 0

#if SCHED_DEBUG
#define SCHED_DBG(fmt, ...)                                                    \
  fprintf(stderr, COLOR_RED "[sched] " fmt "\n" STYLE_RESET_ALL, ##__VA_ARGS__)
#else
#define SCHED_DBG(fmt, ...) ((void)0)
#endif

#ifdef __APPLE__
static int kqueue_fd;
static int wake_pipe[2];
#else
static int timer_fd, wake_fd, epoll_fd;
static const int scheduler_rt_priority = 10;
#endif

static pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool scheduler_fds_ready = false;
static bool scheduler_thread_started = false;

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

typedef struct SchedulerTask {
  bool cancelled;
  bool audio_cancel_sent;
  bool completed;
  double sample_remainder;
  struct SchedulerTask *parent;
  struct SchedulerTask **children;
  size_t num_children;
  size_t children_cap;
  SchedulerCallback callback;
  void *userdata;
} SchedulerTask;

typedef struct {
  void (*callback)(void *userdata, uint64_t now);
  void *userdata;
  SchedulerTask *task;
  uint64_t tick;
  uint64_t dispatch_tick;
  uint64_t sequence;
} SchedulerEvent;

typedef struct {
  SchedulerEvent *events;
  size_t capacity;
  size_t size;
} EventHeap;

#define INITIAL_CAPACITY 64
#define INITIAL_TASK_CAPACITY 16
static const uint64_t scheduler_lookahead_samples =
    BUF_SIZE * SCHEDULER_LOOKAHEAD_SUBBLOCKS;
static uint64_t next_event_sequence = 0;

static bool event_before(const SchedulerEvent *left,
                         const SchedulerEvent *right) {
  if (left->dispatch_tick != right->dispatch_tick) {
    return left->dispatch_tick < right->dispatch_tick;
  }
  return left->sequence < right->sequence;
}

static uint64_t task_dispatch_tick(uint64_t target_tick, uint64_t base_tick) {
  if (target_tick <= base_tick + scheduler_lookahead_samples) {
    return base_tick;
  }

  uint64_t dispatch_tick = target_tick - scheduler_lookahead_samples;
  return dispatch_tick > base_tick ? dispatch_tick : base_tick;
}

static SchedulerTask **scheduler_tasks = NULL;
static size_t scheduler_tasks_size = 0;
static size_t scheduler_tasks_cap = 0;
static _Thread_local SchedulerTask *current_task = NULL;

static void init_task_store(void) {
  if (scheduler_tasks) {
    return;
  }
  scheduler_tasks = malloc(sizeof(SchedulerTask *) * INITIAL_TASK_CAPACITY);
  scheduler_tasks_cap = scheduler_tasks ? INITIAL_TASK_CAPACITY : 0;
  scheduler_tasks_size = 0;
}

static SchedulerTask *find_task(void *handle) {
  SchedulerTask *needle = (SchedulerTask *)handle;
  if (!needle) {
    return NULL;
  }
  for (size_t i = 0; i < scheduler_tasks_size; ++i) {
    if (scheduler_tasks[i] == needle) {
      return scheduler_tasks[i];
    }
  }
  return NULL;
}

static SchedulerTask *create_task(SchedulerCallback callback, void *userdata,
                                  SchedulerTask *parent) {
  init_task_store();
  if (!scheduler_tasks) {
    return NULL;
  }

  if (scheduler_tasks_size >= scheduler_tasks_cap) {
    size_t next_cap =
        scheduler_tasks_cap > 0 ? scheduler_tasks_cap * 2
                                : INITIAL_TASK_CAPACITY;
    SchedulerTask **tasks =
        realloc(scheduler_tasks, sizeof(SchedulerTask *) * next_cap);
    if (!tasks) {
      return NULL;
    }
    scheduler_tasks = tasks;
    scheduler_tasks_cap = next_cap;
  }

  SchedulerTask *task = calloc(1, sizeof(*task));
  if (!task) {
    return NULL;
  }

  task->callback = callback;
  task->userdata = userdata;
  task->parent = parent;
  scheduler_tasks[scheduler_tasks_size++] = task;
  return task;
}

static void task_add_child(SchedulerTask *parent, SchedulerTask *child) {
  if (!parent || !child) {
    return;
  }

  if (parent->num_children >= parent->children_cap) {
    size_t next_cap = parent->children_cap > 0 ? parent->children_cap * 2 : 4;
    SchedulerTask **children =
        realloc(parent->children, sizeof(SchedulerTask *) * next_cap);
    if (!children) {
      return;
    }
    parent->children = children;
    parent->children_cap = next_cap;
  }

  parent->children[parent->num_children++] = child;
}

static void cancel_task_recursive(SchedulerTask *task) {
  if (!task || task->cancelled) {
    return;
  }

  task->cancelled = true;
  for (size_t i = 0; i < task->num_children; ++i) {
    cancel_task_recursive(find_task(task->children[i]));
  }
}

static bool task_is_done(SchedulerTask *task) {
  return task && (task->cancelled || task->completed);
}

static uint64_t scheduler_seconds_to_samples(double seconds) {
  int sr = ctx_sample_rate();
  if (sr <= 0) {
    sr = 48000;
  }
  return seconds > 0.0 ? (uint64_t)(seconds * (double)sr) : 0;
}

static uint64_t task_delay_samples(SchedulerTask *task, double seconds) {
  if (!task || seconds <= 0.0) {
    return 0;
  }

  int sr = ctx_sample_rate();
  if (sr <= 0) {
    sr = 48000;
  }

  double exact = seconds * (double)sr + task->sample_remainder;
  uint64_t samples = (uint64_t)exact;
  task->sample_remainder = exact - (double)samples;
  return samples;
}

void init_heap(EventHeap *heap) {
  if (!heap || heap->events) {
    return;
  }

  heap->events = malloc(sizeof(SchedulerEvent) * INITIAL_CAPACITY);
  heap->capacity = heap->events ? INITIAL_CAPACITY : 0;
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
    if (event_before(&heap->events[index], &heap->events[parent])) {
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
        event_before(&heap->events[left_child], &heap->events[smallest])) {
      smallest = left_child;
    }

    if (right_child < heap->size &&
        event_before(&heap->events[right_child], &heap->events[smallest])) {
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
    SchedulerEvent empty = {
        .callback = NULL,
        .userdata = NULL,
        .task = NULL,
        .tick = 0,
        .dispatch_tick = 0,
        .sequence = 0,
    };
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
#ifdef __APPLE__
  (void)target_tick;
  return;
#else
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
#endif
}

void scheduler_wake() {
  if (!scheduler_fds_ready) {
    return;
  }

#ifdef __APPLE__
  char value = 1;
  write(wake_pipe[1], &value, sizeof(value));
#else
  uint64_t value = 1;
  write(wake_fd, &value, sizeof(value));
#endif
}

static void push_task_event(SchedulerTask *task, uint64_t delay_in_samples,
                            uint64_t target_base, uint64_t dispatch_base) {
  if (!task || task_is_done(task)) {
    return;
  }

  EventHeap *queue = &scheduler_queue;
  pthread_mutex_lock(&scheduler_mutex);
  init_heap(queue);
  if (!queue->events) {
    pthread_mutex_unlock(&scheduler_mutex);
    return;
  }

  if (task_is_done(task)) {
    pthread_mutex_unlock(&scheduler_mutex);
    return;
  }

  if (queue->size >= queue->capacity) {
    size_t next_cap =
        queue->capacity > 0 ? queue->capacity * 2 : INITIAL_CAPACITY;
    SchedulerEvent *events =
        realloc(queue->events, sizeof(SchedulerEvent) * next_cap);
    if (!events) {
      pthread_mutex_unlock(&scheduler_mutex);
      return;
    }
    queue->events = events;
    queue->capacity = next_cap;
  }

  uint64_t target_time = target_base + delay_in_samples;
  uint64_t dispatch_time = task_dispatch_tick(target_time, dispatch_base);

  SchedulerEvent event = {.callback = task->callback,
                          .userdata = task->userdata,
                          .task = task,
                          .tick = target_time,
                          .dispatch_tick = dispatch_time,
                          .sequence = next_event_sequence++};

  queue->events[queue->size] = event;

  heapify_up(queue, queue->size);
  queue->size++;

  bool is_earliest = (queue->events[0].dispatch_tick == dispatch_time);
  pthread_mutex_unlock(&scheduler_mutex);

  if (is_earliest && scheduler_fds_ready) {
    SCHED_DBG("push_task_event: new earliest tick=%llu, arming timer",
              (unsigned long long)dispatch_time);
    arm_timer(dispatch_time);
    scheduler_wake();
  } else {
    SCHED_DBG("push_task_event: tick=%llu (earliest=%llu, heap size=%zu)",
              (unsigned long long)dispatch_time,
              (unsigned long long)queue->events[0].dispatch_tick, queue->size);
  }
}

void push_event(void (*callback)(void *, uint64_t), void *userdata,
                uint64_t delay_in_samples, uint64_t base_time) {

  EventHeap *queue = &scheduler_queue;
  pthread_mutex_lock(&scheduler_mutex);
  init_heap(queue);
  if (!queue->events) {
    pthread_mutex_unlock(&scheduler_mutex);
    return;
  }

  if (queue->size >= queue->capacity) {
    size_t next_cap =
        queue->capacity > 0 ? queue->capacity * 2 : INITIAL_CAPACITY;
    SchedulerEvent *events =
        realloc(queue->events, sizeof(SchedulerEvent) * next_cap);
    if (!events) {
      pthread_mutex_unlock(&scheduler_mutex);
      return;
    }
    queue->events = events;
    queue->capacity = next_cap;
  }

  // Calculate absolute timestamp for next event
  uint64_t target_time = base_time + delay_in_samples;

  SchedulerEvent event = {.callback = callback,
                          .userdata = userdata,
                          .task = NULL,
                          .tick = target_time,
                          .dispatch_tick = target_time,
                          .sequence = next_event_sequence++};

  queue->events[queue->size] = event;

  heapify_up(queue, queue->size);
  queue->size++;

  // did this event become the new earliest?
  bool is_earliest = (queue->events[0].dispatch_tick == target_time);
  pthread_mutex_unlock(&scheduler_mutex);

  if (is_earliest && scheduler_fds_ready) {
    SCHED_DBG("push_event: new earliest tick=%llu, arming timer",
              (unsigned long long)target_time);
    arm_timer(target_time);
    scheduler_wake();
  } else {
    SCHED_DBG("push_event: tick=%llu (earliest=%llu, heap size=%zu)",
              (unsigned long long)target_time,
              (unsigned long long)queue->events[0].dispatch_tick, queue->size);
  }
}
static _Thread_local uint64_t sched_now = 0;
static uint64_t cur_tick = 0;

uint64_t get_tl_tick() {
  return cur_tick == 0 ? get_current_sample() : cur_tick;
}
uint64_t get_sched_tick() { return sched_now; }

void *get_current_task_token(void) { return current_task; }

void scheduler_init_fds() {
  if (scheduler_fds_ready) {
    return;
  }

#ifdef __APPLE__
  kqueue_fd = kqueue();
  if (kqueue_fd < 0 || pipe(wake_pipe) < 0) {
    return;
  }

  fcntl(wake_pipe[0], F_SETFL, O_NONBLOCK);
  fcntl(wake_pipe[1], F_SETFL, O_NONBLOCK);

  struct kevent event;
  EV_SET(&event, wake_pipe[0], EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kqueue_fd, &event, 1, NULL, 0, NULL) < 0) {
    return;
  }

  scheduler_fds_ready = true;
  return;
#else
  timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  wake_fd = eventfd(0, EFD_NONBLOCK);
  epoll_fd = epoll_create1(0);
  if (timer_fd < 0 || wake_fd < 0 || epoll_fd < 0) {
    return;
  }

  struct epoll_event ev = {.events = EPOLLIN};
  ev.data.fd = timer_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev);
  ev.data.fd = wake_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wake_fd, &ev);
  scheduler_fds_ready = true;
#endif
}

// Batch of events to fire outside the lock
#define MAX_BATCH 64
static SchedulerEvent pending_batch[MAX_BATCH];

// Caller must hold scheduler_mutex. Returns count of events collected.
static int collect_due_events(uint64_t current_sample) {
  int count = 0;
  while (scheduler_queue.size > 0 &&
         scheduler_queue.events[0].dispatch_tick <= current_sample &&
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
    if (event.task) {
      pthread_mutex_lock(&scheduler_mutex);
      bool cancelled = task_is_done(event.task);
      pthread_mutex_unlock(&scheduler_mutex);
      if (cancelled) {
        continue;
      }
    }

    if (event.userdata == NULL) {
      current_task = NULL;
      void (*cb)(uint64_t) = (void (*)(uint64_t))event.callback;
      cb(event.tick);
    } else {
      cur_tick = event.tick;
      current_task = event.task;
      event.callback(event.userdata, cur_tick);
      current_task = NULL;
    }
  }
  cur_tick = 0;
}

uint64_t get_current_sample() { return atomic_load(&global_sample_position); }

void *scheduler_thread_fn(void *arg) {
#ifndef __APPLE__
  struct sched_param param = {.sched_priority = scheduler_rt_priority};
  int priority_result =
      pthread_setschedparam(pthread_self(), SCHED_RR, &param);
  if (priority_result != 0) {
    fprintf(stderr, "scheduler: real-time priority unavailable: %s\n",
            strerror(priority_result));
  }
#endif

#ifdef __APPLE__
  SCHED_DBG("thread started, kqueue_fd=%d wake_fd=%d", kqueue_fd,
            wake_pipe[0]);
  struct kevent events[2];
  for (;;) {
    struct timespec timeout;
    struct timespec *timeout_ptr = NULL;

    pthread_mutex_lock(&scheduler_mutex);
    if (scheduler_queue.size > 0) {
      uint64_t now = get_current_sample();
      uint64_t target = scheduler_queue.events[0].dispatch_tick;
      int sample_rate = ctx_sample_rate();
      if (sample_rate <= 0) {
        sample_rate = 48000;
      }
      double seconds = target <= now
                           ? 0.0
                           : (double)(target - now) / sample_rate;
      timeout.tv_sec = (time_t)seconds;
      timeout.tv_nsec =
          (long)((seconds - (double)timeout.tv_sec) * 1e9);
      if (timeout.tv_sec == 0 && timeout.tv_nsec == 0) {
        timeout.tv_nsec = 1;
      }
      timeout_ptr = &timeout;
    }
    pthread_mutex_unlock(&scheduler_mutex);

    int nfds = kevent(kqueue_fd, NULL, 0, events, 2, timeout_ptr);

    if (nfds < 0) {
      perror("[sched] kevent");
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].filter == EVFILT_READ) {
        char buffer[64];
        while (read(wake_pipe[0], buffer, sizeof(buffer)) > 0) {
        }
      }
    }

    sched_now = get_current_sample();
    uint64_t now = sched_now;

    for (;;) {
      pthread_mutex_lock(&scheduler_mutex);
      int count = collect_due_events(now);
      pthread_mutex_unlock(&scheduler_mutex);

      if (count == 0) {
        break;
      }

      fire_events(count);
    }

    pthread_mutex_lock(&scheduler_mutex);
    if (scheduler_queue.size > 0) {
      arm_timer(scheduler_queue.events[0].dispatch_tick);
    }
    pthread_mutex_unlock(&scheduler_mutex);
  }
  return NULL;
#else
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
                    ? (unsigned long long)scheduler_queue.events[0].dispatch_tick
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
                (unsigned long long)scheduler_queue.events[0].dispatch_tick);
      arm_timer(scheduler_queue.events[0].dispatch_tick);
    } else {
      SCHED_DBG("heap empty, sleeping until next push");
    }
    pthread_mutex_unlock(&scheduler_mutex);
  }
  return NULL;
#endif
}

void *schedule_event(uint64_t now, double delay_seconds,
                     SchedulerCallback callback, void *userdata) {
  if (userdata == NULL)
    return userdata;
  uint64_t delay_samps = scheduler_seconds_to_samples(delay_seconds);
  SCHED_DBG("schedule_event: now=%llu delay_sec=%.6f sr=%d delay_samps=%llu",
            (unsigned long long)now, delay_seconds, ctx_sample_rate(),
            (unsigned long long)delay_samps);
  push_event(callback, userdata, delay_samps, now);
  return userdata;
}

void *ylc_schedule_current_task_event(uint64_t now, double delay_seconds) {
  if (!current_task) {
    return NULL;
  }

  pthread_mutex_lock(&scheduler_mutex);
  SchedulerTask *task = find_task(current_task);
  bool can_schedule = task && !task->cancelled && !task->completed;
  uint64_t delay_samps =
      can_schedule ? task_delay_samples(task, delay_seconds) : 0;
  pthread_mutex_unlock(&scheduler_mutex);

  if (!can_schedule) {
    return task;
  }

  /* Prepare future coroutine steps against the scheduler clock.  Using the
     yielded event tick here makes every step in the lookahead immediately
     due and can run an unbounded coroutine chain in one wakeup. */
  push_task_event(task, delay_samps, now, get_sched_tick());
  return task;
}

void ylc_complete_current_task(void) {
  if (!current_task) {
    return;
  }

  pthread_mutex_lock(&scheduler_mutex);
  SchedulerTask *task = find_task(current_task);
  if (task) {
    task->completed = true;
  }
  pthread_mutex_unlock(&scheduler_mutex);
}

void cancel_task(void *handle) {
  if (!handle) {
    return;
  }

  pthread_mutex_lock(&scheduler_mutex);
  SchedulerTask *task = find_task(handle);
  cancel_task_recursive(task);
  uint64_t tick = current_task ? get_tl_tick() : get_current_sample();
  pthread_mutex_unlock(&scheduler_mutex);

  if (!task) {
    return;
  }

  for (size_t i = 0; i < scheduler_tasks_size; ++i) {
    SchedulerTask *cancelled = scheduler_tasks[i];
    if (cancelled->cancelled && !cancelled->audio_cancel_sent) {
      cancel_audio_task(&ctx.msg_queue, cancelled, tick);
      cancelled->audio_cancel_sent = true;
    }
  }
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

void defer_quant_offset(double quant, double offset,
                        DeferQuantCallback callback) {
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

/* Start a `play_pattern` event chain. `callback` is the per-step
   SchedulerCallback (the MIR-lowered `__ylc_play_pattern_step`); `handle` is
   the coroutine handle passed through as userdata. `quant` selects when the
   first step fires: 0 (or negative) fires immediately; a positive value x
   defers to the next multiple of x seconds of `global_sample_position` (i.e.
   since the scheduler/engine started), exactly like defer_quant. Each step then
   reschedules itself with the wait-time yielded by the coroutine. */
void *ylc_play_pattern_start(double quant, SchedulerCallback callback,
                             void *handle) {
  if (!callback || !handle) {
    return NULL;
  }

  pthread_mutex_lock(&scheduler_mutex);
  SchedulerTask *parent = find_task(current_task);
  if (current_task && task_is_done(parent)) {
    pthread_mutex_unlock(&scheduler_mutex);
    return NULL;
  }

  SchedulerTask *task = create_task(callback, handle, parent);
  if (parent && task) {
    task_add_child(parent, task);
  }
  pthread_mutex_unlock(&scheduler_mutex);

  if (!task) {
    return NULL;
  }

  /* A fork created by an early callback inherits its logical event tick.
     Top-level starts still use the live audio clock. */
  uint64_t now = current_task ? get_tl_tick() : get_current_sample();

  double delay_seconds = 0.0;

  if (quant > 0.0) {
    int sr = ctx_sample_rate();
    if (sr == 0) {
      sr = 48000;
    }
    uint64_t quant_samps = (uint64_t)(quant * sr);
    if (quant_samps > 0) {
      uint64_t offset_in_cycle = now % quant_samps;
      uint64_t remainder =
          (offset_in_cycle == 0) ? quant_samps : quant_samps - offset_in_cycle;
      delay_seconds = (double)remainder / (double)sr;
    }
  }

  uint64_t delay_samps = scheduler_seconds_to_samples(delay_seconds);

  push_task_event(task, delay_samps, now, get_sched_tick());
  return task;
}

int scheduler_event_loop() {
  if (scheduler_thread_started) {
    return 0;
  }

  init_heap(&scheduler_queue);
  if (!scheduler_queue.events) {
    return 1;
  }
  scheduler_init_fds();
  if (!scheduler_fds_ready) {
    return 1;
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, scheduler_thread_fn, NULL) != 0) {
    fprintf(stderr, "Failed to create timer thread\n");
    return 1;
  }
  scheduler_thread_started = true;
  return 0;
}
