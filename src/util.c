#ifndef _UTIL
#define _UTIL
#include <errno.h>
#include <stdio.h>

#include <math.h>

#include <stdlib.h>
#include <time.h>

int sleep_millisecs(double msec) {
  int ret;
  struct timespec ts;
  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (fmod(msec, 1000)) * 1000000;
  do {
    ret = nanosleep(&ts, &ts);
  } while (ret && errno == EINTR);
}
#endif
