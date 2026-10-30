#ifndef YLC_SF_BUFFER_H
#define YLC_SF_BUFFER_H

#include "../lang/ylc_datatypes.h"
#include <stdbool.h>

/* Borrowed immutable metadata; the registry owns its storage. */
typedef struct {
  const double *data;
  int frames;
  int channels;
  int samplerate;
} SfBufferView;

void *sf_ref_load(_String path);
bool sf_ref_valid(void *buffer);
const SfBufferView *sf_ref_view(void *buffer);

#endif
