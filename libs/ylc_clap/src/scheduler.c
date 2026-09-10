#include "scheduler.h"

#include <stdlib.h>
#include <string.h>

#define YLC_CLAP_SCHED_INITIAL_CAPACITY 64
#define YLC_CLAP_TASKS_INITIAL_CAPACITY 16

static _Thread_local ylc_clap_scheduler_t *ylc_clap_sched_active = NULL;
static _Thread_local void *ylc_clap_current_task_handle = NULL;

void ylc_clap_scheduler_set_active(ylc_clap_scheduler_t *sched) {
  ylc_clap_sched_active = sched;
}

ylc_clap_scheduler_t *ylc_clap_scheduler_get_active(void) {
  return ylc_clap_sched_active;
}

void ylc_clap_scheduler_init(ylc_clap_scheduler_t *sched, int sample_rate) {
  if (!sched) {
    return;
  }

  sched->events =
      malloc(sizeof(ylc_clap_sched_event_t) * YLC_CLAP_SCHED_INITIAL_CAPACITY);
  sched->capacity = YLC_CLAP_SCHED_INITIAL_CAPACITY;
  sched->size = 0;
  sched->tasks =
      malloc(sizeof(ylc_clap_task_t *) * YLC_CLAP_TASKS_INITIAL_CAPACITY);
  sched->tasks_cap = YLC_CLAP_TASKS_INITIAL_CAPACITY;
  sched->tasks_size = 0;
  pthread_mutex_init(&sched->lock, NULL);
  atomic_init(&sched->sample_position, 0);
  sched->cur_tick = 0;
  sched->sample_rate = sample_rate > 0 ? sample_rate : 48000;
}

void ylc_clap_scheduler_clear(ylc_clap_scheduler_t *sched) {
  if (!sched) {
    return;
  }

  pthread_mutex_lock(&sched->lock);
  sched->size = 0;
  for (size_t i = 0; i < sched->tasks_size; ++i) {
    ylc_clap_task_t *task = sched->tasks[i];
    if (!task) {
      continue;
    }
    free(task->children);
    free(task);
  }
  sched->tasks_size = 0;
  pthread_mutex_unlock(&sched->lock);
}

void ylc_clap_scheduler_destroy(ylc_clap_scheduler_t *sched) {
  if (!sched) {
    return;
  }

  for (size_t i = 0; i < sched->tasks_size; ++i) {
    ylc_clap_task_t *task = sched->tasks[i];
    if (!task) {
      continue;
    }
    free(task->children);
    free(task);
  }
  free(sched->tasks);
  sched->tasks = NULL;
  sched->tasks_cap = 0;
  sched->tasks_size = 0;
  free(sched->events);
  sched->events = NULL;
  sched->capacity = 0;
  sched->size = 0;
  pthread_mutex_destroy(&sched->lock);
}

static void ylc_clap_sched_heap_swap(ylc_clap_sched_event_t *a,
                                     ylc_clap_sched_event_t *b) {
  ylc_clap_sched_event_t tmp = *a;
  *a = *b;
  *b = tmp;
}

static void ylc_clap_sched_heapify_up(ylc_clap_scheduler_t *sched,
                                      size_t index) {
  while (index > 0) {
    size_t parent = (index - 1) / 2;
    if (sched->events[index].tick < sched->events[parent].tick) {
      ylc_clap_sched_heap_swap(&sched->events[index], &sched->events[parent]);
      index = parent;
    } else {
      break;
    }
  }
}

static void ylc_clap_sched_heapify_down(ylc_clap_scheduler_t *sched,
                                        size_t index) {
  for (;;) {
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;
    size_t smallest = index;

    if (left < sched->size &&
        sched->events[left].tick < sched->events[smallest].tick) {
      smallest = left;
    }
    if (right < sched->size &&
        sched->events[right].tick < sched->events[smallest].tick) {
      smallest = right;
    }

    if (smallest != index) {
      ylc_clap_sched_heap_swap(&sched->events[index], &sched->events[smallest]);
      index = smallest;
    } else {
      break;
    }
  }
}

static ylc_clap_sched_event_t ylc_clap_sched_pop(ylc_clap_scheduler_t *sched) {
  ylc_clap_sched_event_t earliest = sched->events[0];
  sched->size--;
  if (sched->size > 0) {
    sched->events[0] = sched->events[sched->size];
    ylc_clap_sched_heapify_down(sched, 0);
  }
  return earliest;
}

static void ylc_clap_sched_push(ylc_clap_scheduler_t *sched,
                                ylc_clap_sched_event_t event) {
  pthread_mutex_lock(&sched->lock);

  if (sched->size >= sched->capacity) {
    sched->capacity *= 2;
    ylc_clap_sched_event_t *events =
        realloc(sched->events, sizeof(ylc_clap_sched_event_t) * sched->capacity);
    if (!events) {
      sched->capacity /= 2;
      pthread_mutex_unlock(&sched->lock);
      return;
    }
    sched->events = events;
  }

  sched->events[sched->size] = event;
  ylc_clap_sched_heapify_up(sched, sched->size);
  sched->size++;

  pthread_mutex_unlock(&sched->lock);
}

static ylc_clap_task_t *ylc_clap_task_find(ylc_clap_scheduler_t *sched,
                                           void *handle) {
  ylc_clap_task_t *needle = (ylc_clap_task_t *)handle;
  for (size_t i = 0; i < sched->tasks_size; ++i) {
    if (sched->tasks[i] == needle) {
      return sched->tasks[i];
    }
  }
  return NULL;
}

static ylc_clap_task_t *ylc_clap_task_create(ylc_clap_scheduler_t *sched,
                                             ylc_clap_sched_callback_t callback,
                                             void *userdata,
                                             ylc_clap_task_t *parent) {
  if (sched->tasks_size >= sched->tasks_cap) {
    sched->tasks_cap *= 2;
    ylc_clap_task_t **tasks =
        realloc(sched->tasks, sizeof(ylc_clap_task_t *) * sched->tasks_cap);
    if (!tasks) {
      sched->tasks_cap /= 2;
      return NULL;
    }
    sched->tasks = tasks;
  }

  ylc_clap_task_t *task = calloc(1, sizeof(*task));
  if (!task) {
    return NULL;
  }
  sched->tasks[sched->tasks_size++] = task;
  task->cancelled = false;
  task->completed = false;
  task->parent = parent;
  task->children = NULL;
  task->num_children = 0;
  task->children_cap = 0;
  task->callback = callback;
  task->userdata = userdata;
  return task;
}

static void ylc_clap_task_add_child(ylc_clap_task_t *parent,
                                    ylc_clap_task_t *child) {
  if (!parent || !child) {
    return;
  }
  if (parent->num_children >= parent->children_cap) {
    parent->children_cap =
        parent->children_cap > 0 ? parent->children_cap * 2 : 4;
    ylc_clap_task_t **children =
        realloc(parent->children,
                sizeof(ylc_clap_task_t *) * parent->children_cap);
    if (!children) {
      parent->children_cap =
          parent->children_cap > 4 ? parent->children_cap / 2 : 0;
      return;
    }
    parent->children = children;
  }
  parent->children[parent->num_children++] = child;
}

static void ylc_clap_task_cancel_recursive(ylc_clap_scheduler_t *sched,
                                            ylc_clap_task_t *task) {
  if (!task || task->cancelled) {
    return;
  }

  task->cancelled = true;

  for (size_t i = 0; i < task->num_children; ++i) {
    ylc_clap_task_t *child = ylc_clap_task_find(sched, task->children[i]);
    if (child) {
      ylc_clap_task_cancel_recursive(sched, child);
    }
  }
}

static bool ylc_clap_task_is_cancelled(ylc_clap_scheduler_t *sched,
                                        void *handle) {
  ylc_clap_task_t *task = ylc_clap_task_find(sched, handle);
  return task != NULL && (task->cancelled || task->completed);
}

static void ylc_clap_sched_push_task(ylc_clap_scheduler_t *sched,
                                     uint64_t now, double delay_seconds,
                                     ylc_clap_task_t *task) {
  if (!sched || !task || task->cancelled || task->completed) {
    return;
  }

  int sr = sched->sample_rate > 0 ? sched->sample_rate : 48000;
  int delay_samps = (int)(delay_seconds * (double)sr);
  if (delay_samps < 0) {
    delay_samps = 0;
  }
  uint64_t target = now + (uint64_t)delay_samps;

  ylc_clap_sched_event_t event = {
      .callback = task->callback,
      .userdata = task->userdata,
      .task = task,
      .tick = target,
  };
  ylc_clap_sched_push(sched, event);
}

void ylc_clap_scheduler_advance(ylc_clap_scheduler_t *sched,
                                uint32_t frames) {
  if (!sched) {
    return;
  }
  atomic_fetch_add_explicit(&sched->sample_position, (unsigned long long)frames,
                            memory_order_relaxed);
}

void ylc_clap_scheduler_drain(ylc_clap_scheduler_t *sched) {
  if (!sched) {
    return;
  }

  uint64_t now = atomic_load_explicit(&sched->sample_position,
                                      memory_order_relaxed);

  for (;;) {
    pthread_mutex_lock(&sched->lock);

    ylc_clap_sched_event_t event = {0};
    bool has_event = false;
    if (sched->size > 0 && sched->events[0].tick <= now) {
      event = ylc_clap_sched_pop(sched);
      has_event = true;
    }

    bool cancelled =
        has_event && event.task &&
        ylc_clap_task_is_cancelled(sched, event.task);

    pthread_mutex_unlock(&sched->lock);

    if (!has_event) {
      break;
    }

    if (cancelled) {
      continue;
    }

    sched->cur_tick = event.tick;
    ylc_clap_current_task_handle = event.task ? event.task : event.userdata;
    if (event.callback) {
      event.callback(event.userdata, event.tick);
    }
    ylc_clap_current_task_handle = NULL;
  }

  sched->cur_tick = 0;
}

void *ylc_clap_schedule_event(uint64_t now, double delay_seconds,
                              ylc_clap_sched_callback_t callback,
                              void *userdata) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched) {
    return userdata;
  }

  int sr = sched->sample_rate > 0 ? sched->sample_rate : 48000;
  int delay_samps = (int)(delay_seconds * (double)sr);
  if (delay_samps < 0) {
    delay_samps = 0;
  }
  uint64_t target = now + (uint64_t)delay_samps;

  ylc_clap_sched_event_t event = {
      .callback = callback, .userdata = userdata, .task = NULL, .tick = target};

  ylc_clap_sched_push(sched, event);
  return userdata;
}

void *ylc_clap_play_pattern_start(double quant,
                                  ylc_clap_sched_callback_t callback,
                                  void *handle) {
  if (!callback || !handle) {
    return NULL;
  }

  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched) {
    return NULL;
  }

  pthread_mutex_lock(&sched->lock);
  ylc_clap_task_t *parent =
      ylc_clap_task_find(sched, ylc_clap_current_task_handle);
  ylc_clap_task_t *task = ylc_clap_task_create(sched, callback, handle, parent);
  if (parent && task) {
    ylc_clap_task_add_child(parent, task);
  }
  pthread_mutex_unlock(&sched->lock);
  if (!task) {
    return NULL;
  }

  uint64_t now =
      atomic_load_explicit(&sched->sample_position, memory_order_relaxed);

  double delay_seconds = 0.0;

  if (quant > 0.0) {
    int sr = sched->sample_rate > 0 ? sched->sample_rate : 48000;
    uint64_t quant_samps = (uint64_t)(quant * (double)sr);
    if (quant_samps > 0) {
      uint64_t offset = now % quant_samps;
      uint64_t remainder = (offset == 0) ? quant_samps : quant_samps - offset;
      delay_seconds = (double)remainder / (double)sr;
    }
  }

  ylc_clap_sched_push_task(sched, now, delay_seconds, task);
  return task;
}

void *ylc_clap_schedule_current_task_event(uint64_t now, double delay_seconds) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched || !ylc_clap_current_task_handle) {
    return NULL;
  }

  pthread_mutex_lock(&sched->lock);
  ylc_clap_task_t *task =
      ylc_clap_task_find(sched, ylc_clap_current_task_handle);
  const bool can_schedule = task && !task->cancelled && !task->completed;
  pthread_mutex_unlock(&sched->lock);

  if (!can_schedule) {
    return task;
  }

  ylc_clap_sched_push_task(sched, now, delay_seconds, task);
  return task;
}

void ylc_clap_complete_current_task(void) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched || !ylc_clap_current_task_handle) {
    return;
  }

  pthread_mutex_lock(&sched->lock);
  ylc_clap_task_t *task =
      ylc_clap_task_find(sched, ylc_clap_current_task_handle);
  if (task) {
    task->completed = true;
  }
  pthread_mutex_unlock(&sched->lock);
}

uint64_t ylc_clap_get_current_sample(void) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched) {
    return 0;
  }
  return atomic_load_explicit(&sched->sample_position, memory_order_relaxed);
}

uint64_t ylc_clap_get_tl_tick(void) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched) {
    return 0;
  }
  return sched->cur_tick != 0 ? sched->cur_tick : ylc_clap_get_current_sample();
}

uint64_t ylc_clap_get_sched_tick(void) {
  return ylc_clap_get_current_sample();
}

int ylc_clap_ctx_sample_rate(void) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched) {
    return 48000;
  }
  return sched->sample_rate > 0 ? sched->sample_rate : 48000;
}

void ylc_clap_cancel_task(void *handle) {
  ylc_clap_scheduler_t *sched = ylc_clap_scheduler_get_active();
  if (!sched || !handle) {
    return;
  }

  pthread_mutex_lock(&sched->lock);
  ylc_clap_task_t *task = ylc_clap_task_find(sched, handle);
  ylc_clap_task_cancel_recursive(sched, task);
  pthread_mutex_unlock(&sched->lock);
}
