#ifndef _BUF_READ
#define _BUF_READ

#include <sndfile.h>
#include <stdlib.h>

struct buf_info {
  double *data;
  int frames;
};
struct buf_info *read_sndfile(char *filename);
#endif
