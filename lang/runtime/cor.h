#include <stdbool.h>
typedef void *(*CoroutineFn)(void *coroutine, void *ret_val);

typedef enum { COR_SIG_CONTINUE, COR_SIG_STOP } coroutine_sig;

typedef struct cor {
  int counter;
  CoroutineFn fn_ptr;
  struct cor *next;
  void *argv;
  coroutine_sig sig;
} cor;

#define MAPSIZE

typedef struct cor_runtime {
} cor_runtime;

cor *cor_next(cor *coroutine, void *ret_val);
cor *cor_init(cor *cor, CoroutineFn fn);
cor *cor_alloc();
cor *cor_defer(cor *this, cor next_struct, void *ret_val);
cor *cor_reset(cor *this, cor next_struct, void *ret_val);
cor *cor_loop(cor *this);

#define YIELD(n, r, c, v)                                                      \
  case n: {                                                                    \
    *r = v;                                                                    \
    return c;                                                                  \
  }

#define YIELD_NEW_COR(n, r, c, f)                                              \
  case n: {                                                                    \
    cor *next = cor_alloc();                                                   \
    *next = *c;                                                                \
    next->counter++;                                                           \
    cor_init(c, (CoroutineFn)f);                                               \
    c->next = next;                                                            \
    c->fn_ptr(c, r);                                                           \
    return c;                                                                  \
  }

#define YIELD_REC_COR(n, r, c, f)                                              \
  case 3: {                                                                    \
    cor_init(c, f);                                                            \
    c->fn_ptr(c, r);                                                           \
    return c;                                                                  \
  }

#define COR_START(cor) switch (cor->counter) {

#define COR_END                                                                \
  default: {                                                                   \
    return NULL;                                                               \
  }                                                                            \
    }

typedef void (*EffectWrapper)(void *ret_val);

struct cor_effect_wrap_state {
  cor *wrapped;
  void (*effect)(void *ret_val);
};

cor *cor_wrap_effect(cor *this, EffectWrapper effect_fn);

struct cor_map_state {
  cor *original_cor;
};

cor *cor_map(cor *this, CoroutineFn map_fn);

cor *cor_loop(cor *instance);

cor *cor_replace(cor *this, cor *other_cor);

cor *cor_stop(cor *this);

cor *empty_coroutine();
