#include <stdint.h>
typedef struct TU {
  enum {
    A,
    B,
    C,
  } tag;
  union {
    int A;
    double B;
    uint64_t C;
  } value;

} TU;

int proc_tu(TU tu) {
  switch (tu.tag) {
  case A: {
    if (tu.value.A == 3) {
      return 4;
    }
    return 1;
  }

  case B: {
    return 2;
  }

  case C: {
    return 3;
  }
  }
}

int main() {
  TU t1 = {A, {.A = 1}};
  TU t2 = {A, {.A = 3}};
  TU t3 = {B, {.B = 2.0}};
  TU t4 = {C, {.C = 200}};
  proc_tu(t1);
  proc_tu(t2);
  proc_tu(t3);
  proc_tu(t4);
}
