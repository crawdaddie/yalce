#include "ylc_datatypes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct List List;

// LLVM generates: %Pat = type { i8, double }
// We must match this EXACTLY - a struct with i8 tag and double storage
typedef struct PatResult {
  int8_t tag;     // 0=Int, 1=Double, 2=List
  double storage; // 8-byte storage for all variants
} PatResult;

typedef struct List {
  PatResult value;
  List *next;
} List;

// LLVM generates: Option of Pat = { i8, %Pat } = { i8, { i8, double } }
typedef struct OptionRes {
  int8_t tag;    // 0=Some, 1=None
  PatResult res; // { i8, double }
} OptionRes;

// Helper functions to construct PatResult values
static inline PatResult PatInt(int32_t val) {
  PatResult r;
  r.tag = 0;
  memcpy(&r.storage, &val, sizeof(int32_t));
  return r;
}

static inline PatResult PatDouble(double val) {
  PatResult r;
  r.tag = 1;
  r.storage = val;
  return r;
}

static inline PatResult PatList(List *val) {
  PatResult r;
  r.tag = 2;
  memcpy(&r.storage, &val, sizeof(List *));
  return r;
}

OptionRes parse_pattern(_String input) {
  printf("parsing '%s'\n", input.chars);
  OptionRes result = {.tag = 1, .res = {.tag = 0, .storage = 0}}; // None
  return result;
}
