#include "soundfile.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

#define MAX_CHANNELS 6
int read_file(const char *filename, Signal *signal, int *sf_sample_rate) {
  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

  printf("filename '%s'\n", filename);

  if (!(infile =
            sf_open(filename, SFM_READ,
                    &sfinfo))) { /* Open failed so print an error message. */
    printf("Not able to open input file %s.\n", filename);
    /* Print the error message from libsndfile. */
    puts(sf_strerror(NULL));
    return 1;
  };

  if (sfinfo.channels > MAX_CHANNELS) {
    printf("Not able to process more than %d channels\n", MAX_CHANNELS);
    sf_close(infile);
    return 1;
  };

  size_t total_size = sfinfo.channels * sfinfo.frames;


  // result->buf =
  double *buf = calloc((int)total_size, sizeof(double));

  sf_read_double(infile, buf, total_size);


  sf_close(infile);
  signal->size = total_size;
  signal->layout = sfinfo.channels;
  signal->buf = buf;
  *sf_sample_rate = sfinfo.samplerate;
  return 0;
};

int read_file_float_deinterleaved(const char *filename, SignalFloatDeinterleaved *signal, int *sf_sample_rate) {
  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

  printf("filename '%s'\n", filename);

  if (!(infile =
            sf_open(filename, SFM_READ,
                    &sfinfo))) { /* Open failed so print an error message. */
    printf("Not able to open input file %s.\n", filename);
    /* Print the error message from libsndfile. */
    puts(sf_strerror(NULL));
    return 1;
  };

  if (sfinfo.channels > MAX_CHANNELS) {
    printf("Not able to process more than %d channels\n", MAX_CHANNELS);
    sf_close(infile);
    return 1;
  };

  size_t total_size = sfinfo.channels * sfinfo.frames;


  // result->buf =
  float *buf = calloc((int)total_size, sizeof(float));

  // TODO: ensure properly deinterleave data
  sf_read_float(infile, buf, total_size);



  sf_close(infile);
  signal->size = total_size;
  signal->layout = sfinfo.channels;
  signal->buf = buf;
  *sf_sample_rate = sfinfo.samplerate;
  return 0;
};
