/* Capture ALSA submissions without a MIDI device. */
#include <assert.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include "../audio_clock.h"
static AudioClock fake_audio = {.sample = 0, .time = 10.032, .sample_rate = 48000};
static snd_seq_real_time_t fake_queue;
static int clock_ready = 1;
static int queue_error;
static double fake_now(void) { return 10.; }
static int fake_read(AudioClock *snapshot) {
  *snapshot = fake_audio;
  return clock_ready ? 0 : -1;
}
static int fake_status(snd_seq_t *seq, int queue, snd_seq_queue_status_t *status) {
  return queue_error;
}
static const snd_seq_real_time_t *fake_time(const snd_seq_queue_status_t *status) {
  return &fake_queue;
}
#define audio_clock_now fake_now
#define audio_clock_read fake_read
#define snd_seq_get_queue_status fake_status
#define snd_seq_queue_status_get_real_time fake_time

static snd_seq_event_t captured[16];
static int count;
static int capture(snd_seq_t *seq, snd_seq_event_t *event) {
  captured[count++] = *event;
  return 0;
}
static int drain(snd_seq_t *seq) { return 0; }
#define snd_seq_event_output capture
#define snd_seq_drain_output drain
#include "../midi_linux.c"
#include "../midi_output.c"

int ctx_sample_rate(void) { return 48000; }
uint64_t get_current_sample(void) { return 0; }
uint64_t get_tl_tick(void) { return 48000; }

int main(void) {
  seq_handle = (snd_seq_t *)1;
  output_port_id = 1;
  output_queue = 2;
  clock_mapped = 0;
  queue_rate = 48000;
  midi_endpoint_t dest = (midi_endpoint_t)(uintptr_t)((24 << 16) | 3);
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) == 0);
  assert(captured[0].queue != SND_SEQ_QUEUE_DIRECT);
  assert((captured[0].flags & SND_SEQ_TIME_STAMP_MASK) == SND_SEQ_TIME_STAMP_REAL);
  count = 0;
  assert(send_note_on(dest, 0, 60, 100) == 0);
  assert(captured[0].dest.client == 24);
  assert(captured[0].dest.port == 3);
  count = 0;
  int endpoint = (int)(uintptr_t)dest;
  assert(midi_note(endpoint, 0, 60., 100, 0.25) == 0);
  assert(midi_note(endpoint, 0, 64., 100, 0.25) == 0);
  assert(count == 4);
  assert(captured[0].type == SND_SEQ_EVENT_NOTEOFF);
  assert(captured[1].type == SND_SEQ_EVENT_NOTEON);
  assert(captured[0].time.time.tv_sec == 1);
  assert(captured[0].time.time.tv_nsec == 282000000);
  assert(captured[1].time.time.tv_sec == 1);
  assert(captured[1].time.time.tv_nsec == 32000000);
  assert(memcmp(&captured[1].time, &captured[3].time,
                sizeof(captured[1].time)) == 0);
  assert(midi_note(endpoint, 16, 60., 100, 0.25) < 0);
  assert(midi_note(endpoint, 0, 60.5, 100, 0.25) < 0);
  assert(midi_note(endpoint, 0, NAN, 100, 0.25) < 0);
  assert(midi_note(endpoint, 0, 60., 128, 0.25) < 0);
  assert(midi_note(endpoint, 0, 60., 100, -1.) < 0);
  assert(midi_note(0, 0, 60., 100, 0.25) < 0);
  assert(count == 4);
  assert(send_midi_start_ts(dest, 48000) == 0);
  assert(send_midi_clock_ts(dest, 48000) == 0);
  assert(captured[4].type == SND_SEQ_EVENT_START);
  assert(captured[5].type == SND_SEQ_EVENT_CLOCK);
  assert(captured[4].dest.client == 24 && captured[4].dest.port == 3);
  assert(captured[5].queue == output_queue);
  assert(memcmp(&captured[1].time, &captured[4].time,
                sizeof(captured[1].time)) == 0);
  assert(memcmp(&captured[4].time, &captured[5].time,
                sizeof(captured[4].time)) == 0);
  assert(send_midi_clock_ts(NULL, 48000) < 0);
  count = 4;
  // New audio snapshots must not change deadlines already assigned to a tick.
  fake_audio.time += 0.1;
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) == 0);
  assert(captured[4].time.time.tv_nsec == 32000000);
  count = 4;
  clock_ready = 0;
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) < 0);
  clock_ready = 1;
  fake_audio.generation++;
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) < 0);
  clock_mapped = 0;
  queue_error = -1;
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) < 0);
  queue_error = 0;
  fake_audio.time = 10.;
  fake_audio.sample_rate = 44100;
  queue_rate = 44100;
  assert(send_note_off_ts(dest, 0, 60, 0, 22050) == 0);
  assert(captured[4].time.time.tv_sec == 0);
  assert(captured[4].time.time.tv_nsec == 500000000);
  clock_mapped = 0;
  fake_audio.sample = 44100;
  assert(send_note_on_ts(dest, 0, 60, 100, 22050) < 0);
  // Calibration is independent of both the audio cursor and queue start time.
  clock_mapped = 0;
  fake_audio.sample = 48000;
  fake_audio.sample_rate = 48000;
  fake_audio.time = 11.032;
  queue_rate = 48000;
  fake_queue.tv_sec = 2;
  fake_queue.tv_nsec = 500000000;
  count = 0;
  assert(send_note_on_ts(dest, 0, 60, 100, 48000) == 0);
  assert(captured[0].time.time.tv_sec == 3);
  assert(captured[0].time.time.tv_nsec == 532000000);
  assert(send_note_on_ts(dest, 0, 60, 100, 0) == 0);
  assert(captured[1].time.time.tv_sec == 2);
  assert(captured[1].time.time.tv_nsec == 532000000);
  puts("MIDI routing, timestamps and validation passed");
  return 0;
}
