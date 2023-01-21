#include "config.h"
#include "user_ctx.h"
#include <jack/jack.h>
#include <jack/midiport.h>
#include <math.h>
#include <stdio.h>

sample_t ramp = 0.0;
sample_t note_on;
unsigned char note = 0;
sample_t note_frqs[128];

void calc_note_frqs(sample_t srate) {
  int i;
  for (i = 0; i < 128; i++) {
    note_frqs[i] =
        (2.0 * 440.0 / 32.0) * pow(2, (((sample_t)i - 9.0) / 12.0)) / srate;
  }
}

/* void write_to_output(sample_t **out, sample_t *buf) { */
/*   for (int ch = 0; ch < NUM_CHANNELS; ch++) { */
/*     out[ch] = buf; */
/*   } */
/* } */

void process_midi(nframes_t nframes, UserCtx *ctx, sample_t **out, int i) {

  void *port_buf = jack_port_get_buffer(ctx->input_port, nframes);

  jack_midi_event_t in_event;
  nframes_t event_index = 0;
  nframes_t event_count = jack_midi_get_event_count(port_buf);

  if (event_count > 1) {
    printf(" midisine: have %d events\n", event_count);
    for (i = 0; i < event_count; i++) {
      jack_midi_event_get(&in_event, port_buf, i);
      printf("    event %d time is %d. 1st byte is 0x%08x\n", i, in_event.time,
             *(in_event.buffer));
    }
  };

  jack_midi_event_get(&in_event, port_buf, 0);

  for (i = 0; i < nframes; i++) {
    if ((in_event.time == i) && (event_index < event_count)) {
      if (((*(in_event.buffer) & 0xf0)) == 0x90) {
        /* note on */
        note = *(in_event.buffer + 1);
        note_on = 1.0;
      } else if (((*(in_event.buffer)) & 0xf0) == 0x80) {
        /* note off */
        note = *(in_event.buffer + 1);
        note_on = 0.0;
      }
      event_index++;
      if (event_index < event_count)
        jack_midi_event_get(&in_event, port_buf, event_index);
    }
    ramp += note_frqs[note];
    ramp = (ramp > 1.0) ? ramp - 2.0 : ramp;
    sample_t sample = note_on * sin(2 * PI * ramp);
    out[0][i] = sample;
    out[1][i] = sample;
  }
}
int callback(nframes_t nframes, void *arg) {
  int i;

  UserCtx *ctx = (UserCtx *)arg;
  void *port_buf = jack_port_get_buffer(ctx->input_port, nframes);

  sample_t *out[NUM_CHANNELS];

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    out[ch] = (sample_t *)jack_port_get_buffer(ctx->output_ports[ch], nframes);
  };
  process_midi(nframes, ctx, out, i);

  process_queue(ctx->msg_queue, ctx->graph);
  Graph *tail = graph_perform(ctx->graph, nframes);
  for (i = 0; i < nframes; i++) {
    out[0][i] = ctx->buses[0][i] + ctx->buses[1][i];
    out[1][i] = ctx->buses[0][i] + ctx->buses[1][i];
  }

  return 0;
}
