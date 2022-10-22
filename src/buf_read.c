#include "buf_read.h"

struct buf_info *read_sndfile(char *filename) {
  SNDFILE *file;
  int fs;
  SF_INFO file_info;

  file = sf_open(filename, SFM_READ, &file_info);

  double *buf = calloc(file_info.frames, sizeof(double));

  sf_count_t frames_read = sf_readf_double(file, buf, file_info.frames);
  struct buf_info *b_info = malloc(sizeof(struct buf_info));
  b_info->data = buf;
  b_info->frames = frames_read;
  return b_info;
}
