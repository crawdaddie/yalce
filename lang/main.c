#include "backend_llvm/jit.h"
#include <pthread.h>
#include <stdbool.h>

int main(int argc, char **argv) {
  pthread_t jit_thread;
  int jit_result;
  return jit(argc, argv);
}
