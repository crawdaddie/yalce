#include "cor.h"
#include <stdio.h>
#include <stdlib.h>

cor *cor_init(cor *cor, CoroutineFn fn) {
  cor->counter = 0;
  cor->fn_ptr = fn;
  return cor;
}
cor *cor_alloc() { return malloc(sizeof(cor)); }

cor *cor_next(cor *coroutine, void *ret_val) {

  cor *res = coroutine->fn_ptr(coroutine, ret_val);

  if (res == NULL && coroutine->next != NULL) {
    coroutine = coroutine->next;
    return cor_next(coroutine, ret_val);
  }

  if (res == NULL) {
    return NULL;
  }

  res->counter++;
  // *coroutine = *res;
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
