#include "soundfile.h"
#include "log.h"
#include <string.h>

#define MAX_CHANNELS 6
int read_file(const char *filename, Signal *result) {
  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

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
  result->buf = malloc(sizeof(double) * total_size);
  result->size = sfinfo.frames;
  result->layout = sfinfo.channels;

  sf_read_double(infile, result->buf, total_size);

  sf_close(infile);

  return 0;
};
