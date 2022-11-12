#include "callback.c"
#include "config.h"
#include "oscilloscope.h"
#include "scheduling.h"
/* #include "synths/kicks.c" */
/* #include "synths/squares.c" */
#include "synths/grains.c"
#include "user_ctx.h"
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "buf_read.h"

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *output_ports[NUM_CHANNELS];

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

int srate(t_nframes nframes, void *arg) {
  printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
  return 0;
}

int main(int argc, char **argv) {
  jack_client_t *client;

  if ((client = jack_client_open("simple-synth", JackNullOption, NULL)) == 0) {
    fprintf(stderr, "jack server not running?\n");
    return 1;
  }


  /* jack_set_sample_rate_callback(client, srate, 0); */

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

  t_queue msg_queue = {0, 0, 100, malloc(sizeof(void *) * 100)};
  UserCtx *ctx = get_user_ctx(input_port, output_ports, &msg_queue);

  /* struct buf_info *amen_buf = read_sndfile("fat_amen_mono_48000.wav"); */
  /* struct buf_info *amen_buf = read_sndfile("kick.wav"); */
  struct buf_info *amen_buf = read_sndfile("Gritch Kick 1.wav");
  printf("buf info %d\n", amen_buf->frames);
  ctx->buffers = realloc(ctx->buffers, sizeof(ctx->buffers) + sizeof(amen_buf));
  ctx->buffers[0] = amen_buf;

  jack_set_process_callback(client, callback, ctx);

  if (jack_activate(client)) {
    fprintf(stderr, "cannot activate client");
    return 1;
  }
  connect_ports(client);
  for (int i = 1; i < argc; i += 1) {
    char *arg = argv[i];
    if (strcmp(arg, "--oscilloscope") == 0) {
      pthread_t win_thread;
      pthread_create(&win_thread, NULL, (void *)oscilloscope_view, (void *)ctx);
    }
  }

  /* run until interrupted */
  Graph *kick_node;
  Graph *square_node;
  t_nframes frame_time = jack_frames_since_cycle_start(client);
  /* add_kick_node_msg(ctx, frame_time, 60.0); */
  /* add_square_node_msg(ctx, frame_time); */
  add_grains_node_msg(ctx, frame_time, 0);
  double r[5] = {1.5, 1.5, 0.5, 0.5};
  int i = 0;
  for (;;) {
    msleep(r[i] * 500);
    /* square_node = ctx->graph->next; */
    /* kick_node = ctx->graph->next; */

    /* nframes_t frame_time = jack_frames_since_cycle_start(client); */
    /* trigger_kick_node_msg(ctx, kick_node, frame_time); */
    /* set_freq_msg(ctx, square_node, frame_time, 440.0); */
    i = (i + 1) % 4;
  };

  pthread_exit(NULL);
  jack_client_close(client);
  exit(0);
}
