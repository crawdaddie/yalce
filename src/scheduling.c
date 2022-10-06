#ifndef _SCHEDULING
#define _SCHEDULING
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

enum { NS_PER_SECOND = 1000000000 };

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

struct timespec get_time(void) {
  struct timespec start;
  clock_gettime(CLOCK_REALTIME, &start);
  return start;
}
double timespec_to_secs(struct timespec ts) {
  return ts.tv_sec + (double)ts.tv_nsec / NS_PER_SECOND;
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

#endif
