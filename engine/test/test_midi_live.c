/* Optional ALSA integration test; sends only to its own virtual receiver. */
#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include "../midi_linux.c"
#include "../midi_output.c"

int ctx_sample_rate(void) { return 48000; }
uint64_t get_current_sample(void) { return 0; }
uint64_t get_tl_tick(void) { return 4800; }

int main(void) {
  snd_seq_t *receiver;
  assert(snd_seq_open(&receiver, "default", SND_SEQ_OPEN_INPUT, 0) == 0);
  snd_seq_set_client_name(receiver, "ylc MIDI test");
  int port = snd_seq_create_simple_port(receiver, "ylc test receiver",
      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
      SND_SEQ_PORT_TYPE_APPLICATION);
  assert(port >= 0);
  int dest = midi_open("ylc test receiver");
  assert(dest != 0);
  assert(midi_note(dest, 0, 60., 100, 0.1) == 0);
  assert(midi_note(dest, 1, 64., 100, 0.1) == 0);
  struct pollfd fd;
  assert(snd_seq_poll_descriptors(receiver, &fd, 1, POLLIN) == 1);
  int attacks = 0;
  int releases = 0;
  for (int i = 0; i < 4; i++) {
    if (snd_seq_event_input_pending(receiver, 0) == 0) {
      assert(poll(&fd, 1, 1000) > 0);
    }
    snd_seq_event_t *event;
    assert(snd_seq_event_input(receiver, &event) >= 0);
    printf("Received event %d channel %d note %d\n", event->type,
            event->data.note.channel, event->data.note.note);
    attacks += event->type == SND_SEQ_EVENT_NOTEON;
    releases += event->type == SND_SEQ_EVENT_NOTEOFF;
    snd_seq_free_event(event);
  }
  assert(attacks == 2 && releases == 2);
  snd_seq_close(receiver);
  puts("ALSA delivered both attacks and releases");
}
