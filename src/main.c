#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "callback.c"
#include "config.h"
#include "kicks.c"
#include "user_ctx.h"
#include "scheduling.c"
#include <jack/jack.h>
#include <jack/midiport.h>

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *output_ports[NUM_CHANNELS];
typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;

jack_port_t *output_ports[NUM_CHANNELS];

void jack_shutdown(void *arg) { exit(1); }
void connect_ports(jack_client_t *client) {
  const char **serverports_names;
  serverports_names = jack_get_ports(client, NULL, NULL, JackPortIsInput);
  for (int i = 0; i < NUM_CHANNELS; i++) {

    printf("server port name %s\n", serverports_names[i]);
    printf("app port name %s\n", jack_port_name(output_ports[i]));
    if (jack_connect(client, jack_port_name(output_ports[i]),
                     serverports_names[i])) {
      printf("Cannot connect output port.\n");
      exit(1);
    }
  };
  free(serverports_names);

  serverports_names = jack_get_ports(client, NULL, NULL, JackPortIsOutput);

  if (jack_connect(client, jack_port_name(input_port), serverports_names[3])) {
    printf("cannot connect midi port\n");
  };
  free(serverports_names);
}

int srate(nframes_t nframes, void *arg) {
  printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
  calc_note_frqs((sample_t)nframes);
  return 0;
}

int main(int narg, char **args) {
  jack_client_t *client;

  if ((client = jack_client_open("simple-synth", JackNullOption, NULL)) == 0) {
    fprintf(stderr, "jack server not running?\n");
    return 1;
  }

  calc_note_frqs(jack_get_sample_rate(client));

  jack_set_sample_rate_callback(client, srate, 0);

  jack_on_shutdown(client, jack_shutdown, 0);

  input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE,
                                  JackPortIsInput, 0);

  for (int i = 0; i < NUM_CHANNELS; i++) {
    int index = i + 1;
    char *port_name = (char *)malloc(10 * sizeof(char));
    sprintf(port_name, "audio_out_%d", i);
    output_ports[i] = jack_port_register(
        client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  }

  queue_t msg_queue = {0, 0, 100, malloc(sizeof(void *) * 100)};
  UserCtx *ctx = get_user_ctx(input_port, output_ports, &msg_queue);
  jack_set_process_callback(client, callback, ctx);

  if (jack_activate(client)) {
    fprintf(stderr, "cannot activate client");
    return 1;
  }
  connect_ports(client);

  /* run until interrupted */
  Graph *kick_node;
  add_kick_node(client, ctx);
  double r[5] = {1.5, 1.5, 0.5, 0.5};
  int i = 0; 
  for (;;) {
    msleep(r[i] * 500);
    kick_node = ctx->graph->next;

    trigger_kick_node(client, ctx, kick_node);
    i = (i + 1) % 4;

  };

  jack_client_close(client);
  exit(0);
}
