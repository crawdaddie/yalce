#include "audio_loop.h"
#include "entry.h"
#include "oscillator.h"
#include "scheduling.h"
#include <unistd.h>

int main(int argc, char **argv) {

  maketable_sin();
  maketable_sq();
  start_audio();
  init_scheduling();
  entry();
  stop_audio();
}
