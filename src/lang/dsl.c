#include "dsl.h"
#include <dlfcn.h>
#include <stdio.h>
void *dll = NULL;
void process_input(const char input[2048], int length) {
  dlerror();
  void *sym = dlsym(RTLD_DEFAULT, input);
  char *error;
  void (*func)();

  if ((error = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", error);
    return;
  }
  printf("input: %s symbol %p\n", input, sym);
  *(void **)(&func) = sym;
  func();
};

void setup_lang_ctx() {
  dll = dlopen("./funcs.so", RTLD_LAZY);
  // dll = dlopen(NULL, RTLD_LAZY);
  printf("loaded dll %p\n", dll);
}
