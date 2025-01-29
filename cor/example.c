#include "cor.h"
#include <stdio.h>
#include <stdlib.h>

cor *foo_rec(cor *this, int *ret_val) {
  switch (this->counter) {
  case 0: {
    *ret_val = 0;
    return this;
  }
  case 1: {
    *ret_val = 1;
    return this;
  }
  case 2: {
    *ret_val = 2;
    return this;
  }

  case 3: {
    cor next = (cor){.counter = 0, .fn_ptr = (CoroutineFn)foo_rec};
    return cor_reset(this, &next, ret_val);
  }

  default: {
    return NULL;
  }
  }
}

cor *foo2(cor *this, int *ret_val) {
  switch (this->counter) {
  case 0: {
    *ret_val = 100;
    return this;
  }
  case 1: {
    *ret_val = 200;
    return this;
  }
  case 2: {
    *ret_val = 300;
    return this;
  }
  default: {
    return NULL;
  }
  }
}

cor *bar(cor *this, int *ret_val) {
  switch (this->counter) {
  case 0: {
    *ret_val = 100;
    return this;
  }
  case 1: {
    *ret_val = 200;
    return this;
  }
  case 2: {
    *ret_val = 300;
    return this;
  }

  case 3: {
    int val = *((int *)this->argv[0]);
    val += 1000;
    *(int *)this->argv[0] = val;
    *ret_val = val;
    return this;
  }

  case 4: {
    int val = *((int *)this->argv[0]);
    val += 1000;
    *(int *)this->argv[0] = val;
    *ret_val = val;
    return this;
  }

  case 5: {
    int val = *((int *)this->argv[0]);
    val += 1000;
    *(int *)this->argv[0] = val;
    *ret_val = val;
    return this;
  }
  default: {
    return NULL;
  }
  }
}

cor *foo(cor *this, int *ret_val) {
  switch (this->counter) {
  case 0: {
    *ret_val = 0;
    return this;
  }
  case 1: {
    *ret_val = 1;
    return this;
  }
  case 2: {
    *ret_val = 2;
    return this;
  }

  case 3: {
    cor next = {.counter = 0, .fn_ptr = (CoroutineFn)foo2};
    return cor_defer(this, &next, ret_val);
  }

  case 4: {
    *ret_val = 3;
    return this;
  }

  default: {
    return NULL;
  }
  }
}

cor *foo_incr(cor *this, int *ret_val) {
  switch (this->counter) {
  case 0: {
    int val = *((int *)this->argv[0]);
    *ret_val = val;
    val += 1;
    *(int *)this->argv[0] = val;
    return this;
  }

  case 1: {

    cor next = (cor){.counter = 0,
                     .fn_ptr = (CoroutineFn)foo_incr,
                     .next = NULL,
                     .argv = this->argv};
    return cor_reset(this, &next, ret_val);
  }

  default: {
    return NULL;
  }
  }
}

int main(int argc, char *argv[]) {
  printf("coroutines\n");

  ({
    cor co = {
        .counter = 0,
        .fn_ptr = (CoroutineFn)foo2,
    };

    int ret;
    while (cor_next(&co, &ret)) {
      printf("cor2 yielded %d\n", ret);
    }

    printf("cor2 finished\n\n");
  });

  ({
    cor co = {
        .counter = 0,
        .fn_ptr = (CoroutineFn)foo,
    };

    int ret;
    while (cor_next(&co, &ret)) {
      printf("cor yielded %d\n", ret);
    }

    printf("cor finished\n\n");
  });

  ({
    cor co = {
        .counter = 0,
        .fn_ptr = (CoroutineFn)foo_rec,
    };

    int ret;
    int count = 0;
    while (cor_next(&co, &ret) && count < 20) {
      printf("cor_rec yielded %d\n", ret);
      count++;
    }
    printf("\n");
  });

  ({
    int val = 2;
    int *vals[1] = {&val};
    cor co = {
        .counter = 0, .fn_ptr = (CoroutineFn)bar, .next = NULL, .argv = vals};

    int ret;
    while (cor_next(&co, &ret)) {
      printf("cor yielded %d state: (%d)\n", ret, *(int *)vals[0]);
    }

    printf("cor finished\n\n");
  });

  ({
    int val = 1;
    int *vals[1] = {&val};
    cor co = {.counter = 0,
              .fn_ptr = (CoroutineFn)foo_incr,
              .next = NULL,
              .argv = vals};

    int ret;
    int count = 0;
    while (cor_next(&co, &ret) && count < 20) {
      printf("incrementing cor yielded %d\n", ret);
      count++;
    }

    printf("incrementing cor finished\n\n");
  });

  return 0;
}
