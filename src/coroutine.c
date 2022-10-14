#include "coroutine.h"

#ifdef __x86_64__
static void _co_rout_entry_point(uint32_t part0, uint32_t part1) {
  union ptr_splitter p;
  p.part[0] = part0;
  p.part[1] = part1;
  co_rout_t *co_rout = p.ptr;
  int return_value = co_rout->function(co_rout);
  co_rout->state = co_rout_FINISHED;
  co_rout_yield(co_rout, return_value);
}
#else
static void _co_rout_entry_point(co_rout_t *co_rout) {
  int return_value = co_rout->function(co_rout);
  co_rout->state = co_rout_FINISHED;
  co_rout_yield(co_rout, return_value);
}
#endif

co_rout_t *co_rout_new(thread_t *thread, co_rout_function_t function) {
  co_rout_t *co_rout = calloc(1, sizeof(*co_rout));

  co_rout->state = co_rout_NEW;
  co_rout->stack = calloc(1, default_stack_size);
  co_rout->thread = thread;
  co_rout->function = function;

  getcontext(&co_rout->context);
  co_rout->context.uc_stack.ss_sp = co_rout->stack;
  co_rout->context.uc_stack.ss_size = default_stack_size;
  co_rout->context.uc_link = 0;

#ifdef __x86_64__
  union ptr_splitter p;
  p.ptr = co_rout;
  makecontext(&co_rout->context, (void (*)())_co_rout_entry_point, 2, p.part[0],
              p.part[1]);
#else
  makecontext(&co_rout->context, (void (*)())_co_rout_entry_point, 1, co_rout);
#endif

  return co_rout;
}

int co_rout_resume(co_rout_t *co_rout) {
  if (co_rout->state == co_rout_NEW)
    co_rout->state = co_rout_RUNNING;
  else if (co_rout->state == co_rout_FINISHED)
    return 0;

  ucontext_t old_context = co_rout->thread->co_rout.caller;
  swapcontext(&co_rout->thread->co_rout.caller, &co_rout->context);
  co_rout->context = co_rout->thread->co_rout.callee;
  co_rout->thread->co_rout.caller = old_context;

  return co_rout->yield_value;
}

void co_rout_yield(co_rout_t *co_rout, int value) {
  co_rout->yield_value = value;
  swapcontext(&co_rout->thread->co_rout.callee, &co_rout->thread->co_rout.caller);
}

void co_rout_free(co_rout_t *co_rout) {
  free(co_rout->stack);
  free(co_rout);
}
/**
* example
int g(co_rout_t *co_rout) {
  int x = 0;
  puts("G: BEFORE YIELD, X = 0");
  x = 42;
  puts("G: BEFORE YIELD, X = 42");
  co_rout_yield(co_rout, 0);
  printf("G: AFTER YIELD, X = %d\n", x);

  return -1337;
}

int f(co_rout_t *co_rout) {

  puts("F: BEFORE YIELD");
  co_rout_yield(co_rout, 42);
  puts("F: AFTER FIRST YIELD");
  co_rout_yield(co_rout, 1337);
  puts("F: AFTER SECOND YIELD");

  return 0;
}

int main(void) {
  thread_t t;
  co_rout_t *co1 = co_rout_new(&t, f);
  printf("f yielded %d\n", co_rout_resume(co1));
  printf("f yielded %d\n", co_rout_resume(co1));
  printf("f yielded %d\n", co_rout_resume(co1));
  puts("DONE");
  co_rout_free(co1);
  return 0;
}
**/
