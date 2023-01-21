#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

typedef struct co_rout_t_ co_rout_t;
typedef struct thread_t_ thread_t;
typedef int (*co_rout_function_t)(co_rout_t *co_rout);

typedef enum { co_rout_NEW, co_rout_RUNNING, co_rout_FINISHED } co_rout_state_t;

#ifdef __x86_64__
union ptr_splitter {
  void *ptr;
  uint32_t part[sizeof(void *) / sizeof(uint32_t)];
};
#endif

struct thread_t_ {
  struct {
    ucontext_t callee, caller;
  } co_rout;
};

struct co_rout_t_ {
  co_rout_state_t state;
  co_rout_function_t function;
  thread_t *thread;

  ucontext_t context;
  char *stack;
  int yield_value;
};

static const int default_stack_size = 4096;

void co_rout_yield(co_rout_t *co_rout, int value);
