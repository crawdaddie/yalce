#ifndef _LANG_YLC_STDLIB_LWT_H
#define _LANG_YLC_STDLIB_LWT_H

typedef struct {
  void *cb;
  struct bound_cbs *next;
} bound_cbs;

typedef struct {
  enum {
    PROMISE_FULFILLED,
    PROMISE_ERROR,
    PROMISE_PENDING,
  } status;
  void *data;
  bound_cbs *bindings;
} Promise;

#endif
