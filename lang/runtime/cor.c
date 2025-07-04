#include "cor.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void *empty_cor_fn(cor *this, void *ret_val) { return NULL; }

cor *empty_coroutine() {
  cor *routine = cor_alloc();
  cor_init(routine, (CoroutineFn)empty_cor_fn);
  return routine;
}

cor *cor_init(cor *cor, CoroutineFn fn) {
  cor->counter = 0;
  cor->fn_ptr = fn;
  return cor;
}
cor *cor_alloc() { return malloc(sizeof(cor)); }

cor *cor_next(cor *coroutine, void *ret_val) {

  if (!coroutine) {
    fprintf(stderr, "Error - coroutine is null\n");
    return NULL;
  }

  if (coroutine->sig == COR_SIG_STOP) {
    return NULL;
  }

  if (coroutine->fn_ptr == NULL) {
    return NULL;
  }

  cor *res = coroutine->fn_ptr(coroutine, ret_val);

  if (res == NULL && coroutine->next != NULL) {
    cor *next = coroutine->next;
    *coroutine = *next;

    return cor_next(coroutine, ret_val);
  }

  if (res == NULL) {
    coroutine->next = NULL;
    return NULL;
  }

  res->counter++;
  *coroutine = *res;
  return res;
}

cor *cor_defer(cor *this, cor next_struct, void *ret_val) {
  cor *next = cor_alloc();
  *next = *this;
  next->counter++;

  *this = next_struct;
  this->next = next;
  this->fn_ptr(this, ret_val);
  return this;
}

cor *cor_reset(cor *this, cor next_struct, void *ret_val) {
  *this = next_struct;
  this->fn_ptr(this, ret_val);
  return this;
}

struct loop_state {
  cor *original_cor;
  void *original_args;
};

void *loop_cor_fn(cor *this, void *ret_val) {
  struct loop_state *state = (struct loop_state *)this->argv;
  cor *inner = state->original_cor;

  cor *res = cor_next(inner, ret_val);

  if (res == NULL) {
    inner->counter = 0;

    if (state->original_args != NULL) {
      inner->argv = state->original_args;
    }

    res = cor_next(inner, ret_val);
  }
  // } else {
  //   printf("why not reset ? %p %p\n", res, ret_val);
  // }

  return this;
}

cor *cor_loop(cor *instance) {
  struct loop_state *state = malloc(sizeof(struct loop_state));
  state->original_cor = instance;
  state->original_args = instance->argv;

  cor mapped_struct = (cor){.counter = 0,
                            .fn_ptr = (CoroutineFn)loop_cor_fn,
                            .next = NULL,
                            .argv = state};

  cor *mapped = cor_alloc();
  *mapped = mapped_struct;

  return mapped;
}

void *effect_wrap(cor *this, void *ret_val) {
  struct cor_effect_wrap_state st = *(struct cor_effect_wrap_state *)this->argv;
  cor *wrapped = st.wrapped;

  cor *res = cor_next(wrapped, ret_val);

  if (res != NULL) {

    st.effect(ret_val);
    return this;
  }
  return NULL;
}

cor *cor_wrap_effect(cor *this, EffectWrapper effect_fn) {
  cor *copy = cor_alloc();
  *copy = *this;

  struct cor_effect_wrap_state st_struct = {.wrapped = copy,
                                            .effect = effect_fn};

  struct cor_effect_wrap_state *st_ptr =
      malloc(sizeof(struct cor_effect_wrap_state));
  *st_ptr = st_struct;

  cor wrapped = (cor){.counter = 0,
                      .fn_ptr = (CoroutineFn)effect_wrap,
                      .next = NULL,
                      .argv = st_ptr};

  // TODO: do I really want to mutate rather than return new copy
  *this = wrapped;
  return this;
}

cor *cor_map(cor *this, CoroutineFn map_fn) {

  cor mapped_struct = (cor){
      .counter = 0, .fn_ptr = (CoroutineFn)map_fn, .next = NULL, .argv = this};
  cor *mapped = cor_alloc();
  *mapped = mapped_struct;

  return mapped;
}

cor *cor_replace(cor *this, cor *other_cor) {
  this->sig = COR_SIG_STOP;
  this->next = NULL;
  this->argv = NULL;
  *this = *other_cor;
  return this;
}

cor *cor_stop(cor *this) {
  this->sig = COR_SIG_STOP;
  this->fn_ptr = NULL;
  this->next = NULL;
  return this;
}
