// add.c
#include <stdio.h>

int add(int first, int second) {
  printf("printing from wasm\n");
  return first + second;
}
