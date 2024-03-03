#include "entry.h"
#include "oscillator.h"
#include "start_audio.h"
#include "window.h"
#include <unistd.h>

int main(int argc, char **argv) {
  window_dsp_setup();

  maketable_sin();
  maketable_sq();
  start_audio();
  entry();
  stop_audio();
}
