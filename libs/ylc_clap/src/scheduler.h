#ifndef YLC_CLAP_SCHEDULER_H
#define YLC_CLAP_SCHEDULER_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ylc_clap_sched_callback_t)(void *userdata, uint64_t tick);

typedef struct ylc_clap_sched_event {
  ylc_clap_sched_callback_t callback;
  void *userdata;
  struct ylc_clap_task *task;
  uint64_t tick;
} ylc_clap_sched_event_t;

typedef struct ylc_clap_task {
  bool cancelled;
  bool completed;
  struct ylc_clap_task *parent;
  struct ylc_clap_task **children;
  size_t num_children;
  size_t children_cap;
  ylc_clap_sched_callback_t callback;
  void *userdata;
} ylc_clap_task_t;

typedef struct ylc_clap_scheduler {
  ylc_clap_sched_event_t *events;
  size_t capacity;
  size_t size;
  pthread_mutex_t lock;
  atomic_ullong sample_position;
  uint64_t cur_tick;
  int sample_rate;
  ylc_clap_task_t **tasks;
  size_t tasks_size;
  size_t tasks_cap;
} ylc_clap_scheduler_t;

void ylc_clap_scheduler_init(ylc_clap_scheduler_t *sched, int sample_rate);
void ylc_clap_scheduler_clear(ylc_clap_scheduler_t *sched);
void ylc_clap_scheduler_destroy(ylc_clap_scheduler_t *sched);

void ylc_clap_scheduler_advance(ylc_clap_scheduler_t *sched, uint32_t frames);
void ylc_clap_scheduler_drain(ylc_clap_scheduler_t *sched);

void ylc_clap_scheduler_set_active(ylc_clap_scheduler_t *sched);
ylc_clap_scheduler_t *ylc_clap_scheduler_get_active(void);

void *ylc_clap_schedule_event(uint64_t now, double delay_seconds,
                              ylc_clap_sched_callback_t callback,
                              void *userdata);
void *ylc_clap_play_pattern_start(double quant,
                                  ylc_clap_sched_callback_t callback,
                                  void *handle);
void *ylc_clap_schedule_current_task_event(uint64_t now, double delay_seconds);
void ylc_clap_complete_current_task(void);

uint64_t ylc_clap_get_current_sample(void);
uint64_t ylc_clap_get_tl_tick(void);
uint64_t ylc_clap_get_sched_tick(void);
int ylc_clap_ctx_sample_rate(void);

void ylc_clap_cancel_task(void *handle);

#ifdef __cplusplus
}
#endif

#endif
