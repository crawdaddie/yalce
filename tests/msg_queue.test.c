#include "../src/msg_queue.h"
#include "../lib/Unity/src/unity.h"
#include "../src/common.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#undef BUF_SIZE
#define BUF_SIZE 16

#define MAX_ERROR_MSG_LEN 100
#define ARRAY_LENGTH(A) (sizeof(A) / sizeof(A[0]))

void setUp(void) {}

void tearDown(void) {}

static void container() { TEST_ASSERT_EQUAL_INT16(16, BUF_SIZE); }

int main(void) {
  UnityBegin("msg_queue.test.c");
  RUN_TEST(container);
  return UnityEnd();
}
