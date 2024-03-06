#include "entry.h"
#include "oscillator.h"
#include "start_audio.h"
#include <unistd.h>

int main(int argc, char **argv) {

  maketable_sin();
  maketable_sq();
  start_audio();
  entry();
  stop_audio();
}
