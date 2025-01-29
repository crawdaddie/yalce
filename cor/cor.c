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
