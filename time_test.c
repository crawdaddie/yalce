#include <stdio.h>
#include <stdlib.h>
double *get_buffer() { return calloc(2048, sizeof(double)); }

int main(void) {
  double *buf = get_buffer();
  double val = 0.0;
  double *valptr = &val;

  printf("buf ptr access %f\n", buf[2045]);
  printf("buf size %d\n", sizeof(buf));

  printf("val ptr size %d\n", sizeof(valptr));
  printf("single val ptr access %f %f %f %f\n", valptr[0], valptr[1], valptr[2],
         valptr[1024]);

  return 0;
}
