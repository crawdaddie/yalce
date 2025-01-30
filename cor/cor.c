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
  struct wrap_state st = *(struct wrap_state *)this->argv;
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

  struct wrap_state st_struct = {
    .wrapped = copy,
    .effect = effect_fn
  };

  struct wrap_state *st_ptr = malloc(sizeof(struct wrap_state));
  *st_ptr = st_struct;

  cor wrapped = (cor){
    .counter = 0,
    .fn_ptr = (CoroutineFn)effect_wrap,
    .next = NULL,
    .argv = st_ptr
  };
  *this = wrapped;
  return this;
}
