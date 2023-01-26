#ifndef _UNIT_TEST_H
#define _UNIT_TEST_H
#include "../dbg-macro/dbg.h"
#include <stdio.h>
#include <stdlib.h>

#define mu_suite_start() char *message = NULL
#define mu_assert(test, message)                                               \
  if (!(test)) {                                                               \
    log_err(message);                                                          \
    return message;                                                            \
  }
#define mu_run_test(test)                                                      \
  debug("\n --------%s", " " #test);                                           \
  message = test();                                                            \
  tests_run++;                                                                 \
  if (message)                                                                 \
    return message;

#define RUN_TEST(name)                                                         \
  int main(int argc, char *argv[]) {                                           \
    argc = 1;                                                                  \
    debug("------------ RUNNING: %s", argv[0]);                                \
    printf("--------\n RUNNING: %s\n", argv[0]);                               \
    char *result = name();                                                     \
    if (result != 0) {                                                         \
      printf("FAILD: %s\n", result);                                           \
    }                                                                          \
    printf("Tests run: %d\n", tests_run);                                      \
    exit(result != 0);                                                         \
  }

int tests_run;

#endif /* end of include guard: MINUNIT_H */
