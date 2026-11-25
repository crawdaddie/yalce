#include "common.h"
#ifndef __APPLE__
#include "midi.h"
#include "ctx.h"
#include "scheduling.h"
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Linux-specific includes
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <time.h>

// MIDI constants
#define CC 0xB0
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define CHAN_MASK 0x0F
#define PROGRAM_CHANGE 0xC0
#define CONTROL_CHANGE 0xB0
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_CONTINUE 0xFB
#define MIDI_STOP 0xFC

#define REC_127 0.007874015748031496

// Global variables
int debug = 0;

// Handler arrays
static CCCallback cc_handlers[128 * 16];
static NoteCallback note_on_handlers[128];
static NoteCallback note_off_handlers[128];
static ProgramChangeCallback program_change_handlers[16];
static TransportCallback midi_clock_handler;
static TransportCallback midi_start_handler;
static TransportCallback midi_continue_handler;
static TransportCallback midi_stop_handler;

// ALSA-specific variables
static snd_seq_t *seq_handle = NULL;
static int client_id;
static int output_port_id = -1;
static int output_queue = -1;
static uint64_t queue_sample;
static int queue_rate;
enum { MIDI_PORT_SHIFT = 16, MIDI_ADDR_MASK = 255, NS_PER_SECOND = 1000000000 };
static int input_port_id;
static pthread_t midi_thread;
static int running = 0;

// Function implementations
void toggle_midi_debug() { debug = !debug; }

// Handler registration functions
void register_cc_handler(int ch, int cc, CCCallback handler) {
  cc_handlers[cc * 16 + ch] = handler;
}

void register_note_on_handler(int ch, NoteCallback handler) {
  note_on_handlers[ch] = handler;
}

void register_note_off_handler(int ch, NoteCallback handler) {
  note_off_handlers[ch] = handler;
}

void register_program_change_handler(int ch, ProgramChangeCallback handler) {
  program_change_handlers[ch] = handler;
}

void register_midi_clock_handler(TransportCallback handler) {
  midi_clock_handler = handler;
}

void register_midi_start_handler(TransportCallback handler) {
  midi_start_handler = handler;
}

void register_midi_continue_handler(TransportCallback handler) {
  midi_continue_handler = handler;
}

void register_midi_stop_handler(TransportCallback handler) {
  midi_stop_handler = handler;
}

// MIDI event handlers
static void handle_midi_event(snd_seq_event_t *ev) {
  switch (ev->type) {
  case SND_SEQ_EVENT_CONTROLLER: {
    CCCallback handler =
        cc_handlers[ev->data.control.param * 16 + ev->data.control.channel];
    if (debug) {
      printf("midi cc ch: %d cc: %d val: %d\n", ev->data.control.channel,
             ev->data.control.param, ev->data.control.value);
    }
    if (handler != NULL) {
      handler((double)(ev->data.control.value * REC_127));
    }
  } break;

  case SND_SEQ_EVENT_NOTEON: {
    NoteCallback handler = note_on_handlers[ev->data.note.channel];
    if (debug) {
      printf("midi note on ch: %d note: %d vel: %d\n", ev->data.note.channel,
             ev->data.note.note, ev->data.note.velocity);
    }
    if (handler != NULL && ev->data.note.velocity > 0) {
      handler(ev->data.note.note, (double)(ev->data.note.velocity * REC_127));
    }
  } break;

  case SND_SEQ_EVENT_NOTEOFF: {
    NoteCallback handler = note_off_handlers[ev->data.note.channel];
    if (debug) {
      printf("midi note off ch: %d note: %d vel: %d\n", ev->data.note.channel,
             ev->data.note.note, ev->data.note.velocity);
    }
    if (handler != NULL) {
      handler(ev->data.note.note, (double)(ev->data.note.velocity * REC_127));
    }
  } break;

  case SND_SEQ_EVENT_PGMCHANGE: {
    ProgramChangeCallback handler =
        program_change_handlers[ev->data.control.channel];
    if (debug) {
      printf("midi program change ch: %d program: %d\n",
             ev->data.control.channel, ev->data.control.value);
    }
    if (handler != NULL) {
      handler(ev->data.control.channel, ev->data.control.value);
    }
  } break;

  case SND_SEQ_EVENT_CLOCK:
    if (debug) {
    } // Don't spam debug with clock
    if (midi_clock_handler)
      midi_clock_handler();
    break;

  case SND_SEQ_EVENT_START:
    if (debug)
      printf("midi start\n");
    if (midi_start_handler)
      midi_start_handler();
    break;

  case SND_SEQ_EVENT_CONTINUE:
    if (debug)
      printf("midi continue\n");
    if (midi_continue_handler)
      midi_continue_handler();
    break;

  case SND_SEQ_EVENT_STOP:
    if (debug)
      printf("midi stop\n");
    if (midi_stop_handler)
      midi_stop_handler();
    break;
  }
}

// MIDI input thread
static void *midi_input_thread(void *arg) {
  snd_seq_event_t *ev;
  while (running) {
    if (snd_seq_event_input(seq_handle, &ev) >= 0) {
      handle_midi_event(ev);
      snd_seq_free_event(ev);
    }
  }
  return NULL;
}

// Setup functions
void midi_setup() {
  if (seq_handle != NULL) {
    return;
  }

  // Initialize handler arrays
  memset(cc_handlers, 0, sizeof(cc_handlers));
  memset(note_on_handlers, 0, sizeof(note_on_handlers));
  memset(note_off_handlers, 0, sizeof(note_off_handlers));
  memset(program_change_handlers, 0, sizeof(program_change_handlers));

  if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    printf("Error opening ALSA sequencer\n");
    return;
  }

  snd_seq_set_client_name(seq_handle, "MIDI Client");
  client_id = snd_seq_client_id(seq_handle);

  input_port_id = snd_seq_create_simple_port(seq_handle, "Input Port",
                                             SND_SEQ_PORT_CAP_WRITE |
                                                 SND_SEQ_PORT_CAP_SUBS_WRITE,
                                             SND_SEQ_PORT_TYPE_MIDI_GENERIC);

  if (input_port_id < 0) {
    printf("Error creating MIDI input port\n");
    return;
  }

  running = 1;
  if (pthread_create(&midi_thread, NULL, midi_input_thread, NULL) != 0) {
    printf("Error creating MIDI input thread\n");
    running = 0;
    return;
  }

  printf("ALSA MIDI client created with ID %d\n", client_id);
  printf("MIDI input port created with ID %d\n", input_port_id);
  printf("For MIDI input only: aconnect <source_client:port> %d:%d\n",
         client_id, input_port_id);
}

void midi_out_setup() {
  if (output_port_id >= 0) {
    return;
  }

  if (seq_handle == NULL) {
    midi_setup();
  }

  if (seq_handle == NULL) {
    return;
  }

  output_port_id = snd_seq_create_simple_port(seq_handle, "Output Port",
                                              SND_SEQ_PORT_CAP_READ |
                                                  SND_SEQ_PORT_CAP_SUBS_READ,
                                              SND_SEQ_PORT_TYPE_MIDI_GENERIC);

  if (output_port_id < 0) {
    printf("Error creating MIDI output port\n");
    return;
  }

  output_queue = snd_seq_alloc_named_queue(seq_handle, "ylc sample timeline");
  queue_rate = ctx_sample_rate();
  queue_sample = get_current_sample();
  if (output_queue < 0 || queue_rate <= 0 ||
      snd_seq_start_queue(seq_handle, output_queue, NULL) < 0 ||
      snd_seq_drain_output(seq_handle) < 0) {
    if (output_queue >= 0) {
      snd_seq_free_queue(seq_handle, output_queue);
    }
    snd_seq_delete_simple_port(seq_handle, output_port_id);
    output_queue = -1;
    output_port_id = -1;
    fprintf(stderr, "Cannot start MIDI output queue\n");
    return;
  }

  printf("MIDI output port created with ID %d\n", output_port_id);
}

int midi_connect_output(midi_endpoint_t destination) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }
  // ALSA hardware ports open their raw MIDI stream on subscription.
  uintptr_t address = (uintptr_t)destination;
  int result = snd_seq_connect_to(seq_handle, output_port_id,
      (address >> MIDI_PORT_SHIFT) & MIDI_ADDR_MASK, address & MIDI_ADDR_MASK);
  if (result < 0 && result != -EBUSY) {
    fprintf(stderr, "Cannot connect MIDI output: %s\n", snd_strerror(result));
    return -1;
  }
  return 0;
}

// Keep one mapping so equal sample ticks always produce equal deadlines.
static int schedule_midi(snd_seq_event_t *event, uint64_t tick) {
  if (output_queue < 0 || queue_rate <= 0 || tick < queue_sample) {
    return -1;
  }
  // shortcut: nominal audio rate; presentation-clock drift needs calibration.
  uint64_t samples = tick - queue_sample;
  uint64_t seconds = samples / queue_rate;
  if (seconds > UINT32_MAX) {
    return -1;
  }
  snd_seq_real_time_t time = {
      .tv_sec = seconds,
      .tv_nsec = (samples % queue_rate) * NS_PER_SECOND / queue_rate};
  snd_seq_ev_schedule_real(event, output_queue, 0, &time);
  return 0;
}

static void route_midi(snd_seq_event_t *event, midi_endpoint_t destination) {
  uintptr_t address = (uintptr_t)destination;
  snd_seq_ev_set_dest(event, (address >> MIDI_PORT_SHIFT) & MIDI_ADDR_MASK,
                      address & MIDI_ADDR_MASK);
}

// MIDI output functions
int send_note_on(midi_endpoint_t destination, char channel, char note,
                 char velocity) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_noteon(&ev, channel, note, velocity);

  if (debug) {
    printf("Sending note on: ch=%u note=%u vel=%u\n", channel, note, velocity);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_note_off(midi_endpoint_t destination, uint8_t channel, uint8_t note,
                  uint8_t velocity) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_noteoff(&ev, channel, note, velocity);

  if (debug) {
    printf("send note off %d %d %d\n", channel, note, velocity);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_note_on_ts(midi_endpoint_t destination, char channel, char note,
                    char velocity, uint64_t ts) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  if (schedule_midi(&ev, ts) < 0) {
    return -1;
  }
  snd_seq_ev_set_noteon(&ev, channel, note, velocity);

  if (debug) {
    printf("Sending note on: ch=%u note=%u vel=%u ts=%" PRIu64 "\n", channel,
           note, velocity, ts);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_note_off_ts(midi_endpoint_t destination, uint8_t channel, uint8_t note,
                     uint8_t velocity, uint64_t ts) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  if (schedule_midi(&ev, ts) < 0) {
    return -1;
  }
  snd_seq_ev_set_noteoff(&ev, channel, note, velocity);

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_note_on_dur_ts(midi_endpoint_t destination, char channel, char note,
                        char velocity, double dur, uint64_t ts) {
  if (!isfinite(dur) || dur <= 0. || queue_rate <= 0 ||
      dur * queue_rate < 1. || dur * queue_rate >= (double)(UINT64_MAX - ts)) {
    return -1;
  }
  // Submit the release first: a failed attack cannot leave a stuck note.
  int result = send_note_off_ts(destination, channel, note, 0,
                                ts + (uint64_t)(dur * queue_rate));
  if (result < 0) {
    return result;
  }
  return send_note_on_ts(destination, channel, note, velocity, ts);
}

int send_cc(midi_endpoint_t destination, char channel, char control_number,
            char value) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_controller(&ev, channel, control_number, value);

  if (debug) {
    printf("Sending CC: ch=%u cc=%u val=%u\n", channel, control_number, value);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_program_change(midi_endpoint_t destination, uint8_t channel,
                        uint8_t program) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_pgmchange(&ev, channel, program);

  if (debug) {
    printf("Sending program change: ch=%u program=%u\n", channel, program);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_program_change_ts(midi_endpoint_t destination, uint8_t channel,
                           uint8_t program, uint64_t ts) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  if (schedule_midi(&ev, ts) < 0) {
    return -1;
  }
  snd_seq_ev_set_pgmchange(&ev, channel, program);

  if (debug) {
    printf("Sending program change: ch=%u program=%u\n", channel, program);
  }

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

// Bulk sending functions
int send_note_ons(midi_endpoint_t destination, int size, char *note_data_ptr) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel = *(note_data_ptr + (i * 3));
    uint8_t note = *(note_data_ptr + (i * 3) + 1);
    uint8_t velocity = *(note_data_ptr + (i * 3) + 2);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, output_port_id);
    route_midi(&ev, destination);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_noteon(&ev, channel, note, velocity);

    if (debug) {
      printf("midi packet midi_data[%d %d %d]\n", NOTE_ON | channel, note,
             velocity);
    }

    snd_seq_event_output(seq_handle, &ev);
  }

  snd_seq_drain_output(seq_handle);
  return 0;
}

int send_note_offs(midi_endpoint_t destination, int size, char *note_data_ptr) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel = *(note_data_ptr + (i * 3));
    uint8_t note = *(note_data_ptr + (i * 3) + 1);
    uint8_t velocity = *(note_data_ptr + (i * 3) + 2);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, output_port_id);
    route_midi(&ev, destination);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_noteoff(&ev, channel, note, velocity);

    snd_seq_event_output(seq_handle, &ev);
  }

  snd_seq_drain_output(seq_handle);
  return 0;
}

int send_ccs(midi_endpoint_t destination, int size, char *cc_data_ptr) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel = *(cc_data_ptr + (i * 3));
    uint8_t control_number = *(cc_data_ptr + (i * 3) + 1);
    uint8_t value = *(cc_data_ptr + (i * 3) + 2);

    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, output_port_id);
    route_midi(&ev, destination);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_controller(&ev, channel, control_number, value);

    snd_seq_event_output(seq_handle, &ev);
  }

  snd_seq_drain_output(seq_handle);
  return 0;
}

// Queue transport beside notes so scheduler lookahead cannot send it early.
static int send_transport_ts(midi_endpoint_t destination, int type,
                              uint64_t tick) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }
  snd_seq_event_t event;
  snd_seq_ev_clear(&event);
  snd_seq_ev_set_source(&event, output_port_id);
  route_midi(&event, destination);
  if (schedule_midi(&event, tick) < 0) {
    return -1;
  }
  event.type = type;
  int result = snd_seq_event_output(seq_handle, &event);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_midi_start_ts(midi_endpoint_t destination, uint64_t tick) {
  return send_transport_ts(destination, SND_SEQ_EVENT_START, tick);
}

int send_midi_clock_ts(midi_endpoint_t destination, uint64_t tick) {
  return send_transport_ts(destination, SND_SEQ_EVENT_CLOCK, tick);
}

// Transport functions
int send_midi_start(midi_endpoint_t destination) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_START;

  if (debug)
    printf("Sending MIDI start\n");

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_midi_stop(midi_endpoint_t destination) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_STOP;

  if (debug)
    printf("Sending MIDI stop\n");

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_midi_continue(midi_endpoint_t destination) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_CONTINUE;

  if (debug)
    printf("Sending MIDI continue\n");

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

int send_midi_clock(midi_endpoint_t destination) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return -1;
  }

  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, output_port_id);
  route_midi(&ev, destination);
  snd_seq_ev_set_direct(&ev);
  ev.type = SND_SEQ_EVENT_CLOCK;

  int result = snd_seq_event_output(seq_handle, &ev);
  int drained = snd_seq_drain_output(seq_handle);
  return result < 0 || drained < 0 ? -1 : 0;
}

// Device management functions
static midi_endpoint_t find_destination(midi_count_t index, const char *name) {
  if (seq_handle == NULL) {
    return NULL;
  }

  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;

  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);

  snd_seq_client_info_set_client(cinfo, -1);
  while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
    int client = snd_seq_client_info_get_client(cinfo);

    snd_seq_port_info_set_client(pinfo, client);
    snd_seq_port_info_set_port(pinfo, -1);
    while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
      const char *port_name = snd_seq_port_info_get_name(pinfo);
      unsigned int caps = snd_seq_port_info_get_capability(pinfo);
      if (client == client_id || !(caps & SND_SEQ_PORT_CAP_WRITE) ||
          !(caps & SND_SEQ_PORT_CAP_SUBS_WRITE)) {
        continue;
      }
      if (name ? strcmp(port_name, name) == 0 : index-- == 0) {
        // Return a composite value encoding client:port
        int port = snd_seq_port_info_get_port(pinfo);
        return (midi_endpoint_t)(uintptr_t)((client << 16) | port);
      }
    }
  }
  return NULL;
}

midi_endpoint_t get_destination(midi_count_t index) {
  return find_destination(index, NULL);
}

midi_endpoint_t get_destination_by_name(const char *name) {
  if (name == NULL) {
    return NULL;
  }
  return find_destination(0, name);
}

void list_destinations() {
  if (seq_handle == NULL) {
    printf("MIDI not initialized\n");
    return;
  }

  printf("ALSA MIDI destinations:\n");

  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;

  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);

  snd_seq_client_info_set_client(cinfo, -1);
  int dest_count = 0;

  while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
    int client = snd_seq_client_info_get_client(cinfo);

    // Skip our own client
    if (client == client_id)
      continue;

    snd_seq_port_info_set_client(pinfo, client);
    snd_seq_port_info_set_port(pinfo, -1);
    while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
      unsigned int capability = snd_seq_port_info_get_capability(pinfo);
      if ((capability & SND_SEQ_PORT_CAP_WRITE) &&
          (capability & SND_SEQ_PORT_CAP_SUBS_WRITE)) {

        const char *client_name = snd_seq_client_info_get_name(cinfo);
        const char *port_name = snd_seq_port_info_get_name(pinfo);
        int port = snd_seq_port_info_get_port(pinfo);

        printf("MIDI Destination %d: %d:%d (%s:%s)\n", dest_count++, client,
               port, client_name, port_name);
      }
    }
  }

  if (dest_count == 0) {
    printf("No MIDI destinations found\n");
  }
}

// Raw data sending
void send_data(midi_endpoint_t destination, size_t size, char *data) {
  if (seq_handle == NULL || output_port_id < 0 || destination == NULL) {
    return;
  }

  // For ALSA, parse the raw data and send appropriate events
  for (int i = 0; i < size; i += 3) {
    if (i + 2 < size) {
      uint8_t status = data[i];
      uint8_t data1 = data[i + 1];
      uint8_t data2 = data[i + 2];

      snd_seq_event_t ev;
      snd_seq_ev_clear(&ev);
      snd_seq_ev_set_source(&ev, output_port_id);
      route_midi(&ev, destination);
      snd_seq_ev_set_direct(&ev);

      // Parse MIDI message type and set appropriate event
      switch (status & 0xF0) {
      case NOTE_ON:
        snd_seq_ev_set_noteon(&ev, status & 0x0F, data1, data2);
        break;
      case NOTE_OFF:
        snd_seq_ev_set_noteoff(&ev, status & 0x0F, data1, data2);
        break;
      case CONTROL_CHANGE:
        snd_seq_ev_set_controller(&ev, status & 0x0F, data1, data2);
        break;
      default:
        continue; // Skip unhandled message types
      }

      snd_seq_event_output(seq_handle, &ev);
    }
  }

  snd_seq_drain_output(seq_handle);
}
#endif
