#include "scheduling.h"
#include "audio_loop.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
// static struct timespec start_time;

static struct timespec block_time;
static struct timespec start_time;
void get_block_time(struct timespec *time) { *time = block_time; }

void set_block_time() {
  clock_gettime(CLOCK_REALTIME, &block_time);
  if (start_time.tv_sec == 0) {
    start_time.tv_sec = block_time.tv_sec;
    start_time.tv_nsec = block_time.tv_nsec;
  }
}
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td) {
  td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
  td->tv_sec = t2.tv_sec - t1.tv_sec;
  if (td->tv_sec > 0 && td->tv_nsec < 0) {
    td->tv_nsec += NS_PER_SECOND;
    td->tv_sec--;
  } else if (td->tv_sec < 0 && td->tv_nsec > 0) {
    td->tv_nsec -= NS_PER_SECOND;
    td->tv_sec++;
  }
}

// returns the difference between two timespecs in seconds
double timespec_diff(struct timespec a, struct timespec b) {
  long sec = (long)(a.tv_sec - b.tv_sec);

  long nsec = a.tv_nsec - b.tv_nsec;

  if (nsec < 0) {
    --sec;
    nsec += 1000000000L;
  }
  return sec + (double)nsec / 1000000000L;
}

double get_time(void) {
  struct timespec current;
  clock_gettime(CLOCK_REALTIME, &current);

  return timespec_diff(current, start_time);
}

double timespec_to_secs(struct timespec ts) {
  return ts.tv_sec;
  // + ((double)ts.tv_nsec / NS_PER_SECOND);
}

int msleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
    res = nanosleep(&ts, NULL);
  } while (res && errno == EINTR);

  return res;
}

int msleepd(double msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = fmod(msec, 1000) * 1000000;

  do {
    res = nanosleep(&ts, NULL);
  } while (res && errno == EINTR);

  return res;
}

void init_scheduling() { clock_gettime(CLOCK_REALTIME, &start_time); }

/*
 * get difference between now and start of current audio block
 * in seconds
 * */
double get_block_diff() {
  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);

  struct timespec audio_block_time;

  // get_block_time(&audio_block_time);
  get_block_time(&audio_block_time);

  return timespec_diff(current_time, audio_block_time);
}

void print_block_time() {
  struct timespec current_time;
  get_block_time(&current_time);
  clock_gettime(CLOCK_REALTIME, &current_time);

  double t = timespec_diff(current_time, start_time);
  printf("time: %f %f\n", t, fmod(t, 0.5));
}

void handle_scheduler_tick() { print_block_time(); }
