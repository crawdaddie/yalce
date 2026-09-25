#include "../audio_loop_utils.h"
#include <assert.h>

int main(void) {
  assert(get_output_buf_frames(0.0213, 48000) >= 1024);
  return 0;
}
