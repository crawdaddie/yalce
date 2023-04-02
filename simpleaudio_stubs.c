#include "src/export.h"
#include <caml/mlvalues.h>
#include <stdio.h>

CAMLprim value start_audio() {
  int audio_status = setup_audio();

  printf("%s\n", audio_status == 0 ? "audio started" : "audio failed");

  return Val_int(audio_status);
}

CAMLprim value kill_audio() {
  int audio_status = stop_audio();

  printf("%s\n", audio_status == 0 ? "audio killed" : "kill audio failed");

  return Val_int(audio_status);
}
