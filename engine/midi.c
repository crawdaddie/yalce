#include "midi.h"
#include "audio_loop.h"
#include "ctx.h"
#include "scheduling.h"
// Add these at the top of your file
#include <AudioToolbox/AudioToolbox.h>
#include <mach/mach_time.h>

#include <CoreMIDI/CoreMIDI.h>

#define CC 0xB0
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define CHAN_MASK 0x0F
#define PROGRAM_CHANGE 0xC0
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_CONTINUE 0xFB
#define MIDI_STOP 0xFC

#define REC_127 0.007874015748031496

int debug;
void toggle_midi_debug() { debug = !debug; }

// Arrays to store handlers
static CCCallback cc_handlers[128 * 16];
static NoteCallback note_on_handlers[128];
static NoteCallback note_off_handlers[128];
static MIDIPortRef output_port;
static MIDIClientRef client;

// Add these callback type definitions after the existing ones
typedef void (*ProgramChangeCallback)(int channel, int program);
typedef void (*TransportCallback)(void);

// Add these arrays after the existing handler arrays
static ProgramChangeCallback program_change_handlers[16];
static TransportCallback midi_clock_handler;
static TransportCallback midi_start_handler;
static TransportCallback midi_continue_handler;
static TransportCallback midi_stop_handler;

// Add these registration functions after the existing ones
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

// Add these handler functions after the existing handle_* functions
static void handle_program_change(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t program = *(packet->data + 1) & 0x7F; // Program numbers are 0-127
  ProgramChangeCallback handler = program_change_handlers[ch];

  if (debug) {
    printf("midi program change ch: %d program: %d\n", ch, program);
  }
  if (handler != NULL) {
    handler(ch, program);
  }
}

static void handle_transport(MIDIPacket *packet) {
  uint8_t status = *packet->data;

  if (debug) {
    switch (status) {
    case MIDI_CLOCK:
      // ignore
      break;
    case MIDI_START:
      printf("midi start\n");
      break;
    case MIDI_CONTINUE:
      printf("midi continue\n");
      break;
    case MIDI_STOP:
      printf("midi stop\n");
      break;
    }
  }

  switch (status) {
  case MIDI_CLOCK:
    if (midi_clock_handler != NULL) {
      midi_clock_handler();
    }
    break;
  case MIDI_START:
    if (midi_start_handler != NULL) {
      midi_start_handler();
    }
    break;
  case MIDI_CONTINUE:
    if (midi_continue_handler != NULL) {
      midi_continue_handler();
    }
    break;
  case MIDI_STOP:
    if (midi_stop_handler != NULL) {
      midi_stop_handler();
    }
    break;
  }
}

void register_cc_handler(int ch, int cc, CCCallback handler) {
  cc_handlers[cc * 16 + ch] = handler;
}

void register_note_on_handler(int ch, NoteCallback handler) {
  note_on_handlers[ch] = handler;
}

void register_note_off_handler(int ch, NoteCallback handler) {
  note_off_handlers[ch] = handler;
}

static void handle_cc(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t cc = *(packet->data + 1) & 0xFF;
  uint8_t val = *(packet->data + 2);
  CCCallback handler = cc_handlers[cc * 16 + ch];

  if (debug) {
    printf("midi cc ch: %d cc: %d val: %d\n", ch, cc, val);
  }
  if (handler != NULL) {
    handler((double)(val * REC_127));
  }
}

static void handle_note_on(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t note = *(packet->data + 1) & 0xFF;
  uint8_t velocity = *(packet->data + 2);
  NoteCallback handler = note_on_handlers[ch];

  if (debug) {
    printf("midi note on ch: %d note: %d vel: %d\n", ch, note, velocity);
  }
  if (handler != NULL) {
    if (velocity > 0) {
      handler(note, (double)(velocity * REC_127));
    }
  }
}

static void handle_note_off(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t note = *(packet->data + 1) & 0xFF;
  uint8_t velocity = *(packet->data + 2);
  NoteCallback handler = note_off_handlers[ch];
  if (debug) {
    printf("midi note off ch: %d note: %d vel: %d\n", ch, note, velocity);
  }
  if (handler != NULL) {
    handler(note, (double)(velocity * REC_127));
  }

  // printf("Note Off: %d %d %d\n", ch, note, velocity);
}

static void MIDIInputCallback(const MIDIPacketList *pktlist,
                              void *readProcRefCon, void *srcConnRefCon) {

  MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
  for (int i = 0; i < pktlist->numPackets; i++) {
    uint8_t status = *packet->data;
    if (status >= 0xF8) {
      handle_transport(packet);
    } else {
      // Handle channel messages
      switch (status & 0xF0) {
      case CC:
        handle_cc(packet);
        break;
      case NOTE_ON:
        handle_note_on(packet);
        break;
      case NOTE_OFF:
        handle_note_off(packet);
        break;
      case PROGRAM_CHANGE:
        handle_program_change(packet);
        break;
      default:
        break;
      }
    }
    packet = MIDIPacketNext(packet);
  }
}
// Check for system real-time messages first (0xF8-0xFF)

static double sample_to_ns_ratio;
static mach_timebase_info_data_t timebase_info;
static uint64_t audio_start_mach_time;

// Convert timespec to mach absolute time
uint64_t timespec_to_mach_time(struct timespec ts) {
  // Convert timespec to nanoseconds
  uint64_t nanoseconds =
      (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

  // Convert nanoseconds to mach time units using the timebase info
  return nanoseconds * timebase_info.denom / timebase_info.numer;
}
// Initialize with your timespec reference and sample rate
void init_midi_timing(struct timespec audio_start_time) {
  mach_timebase_info(&timebase_info);

  sample_to_ns_ratio = 1000000000.0 / 48000;
  audio_start_mach_time = timespec_to_mach_time(audio_start_time);
}

MIDITimeStamp sample_to_midi_timestamp(uint64_t sample_position) {
  // Convert samples to nanoseconds
  double nanoseconds = sample_position * sample_to_ns_ratio;

  // Convert nanoseconds to mach time units
  uint64_t sample_mach_offset =
      (uint64_t)nanoseconds * timebase_info.denom / timebase_info.numer;

  // Return audio start time plus sample offset
  return audio_start_mach_time + sample_mach_offset;
}

void midi_setup() {
  // Initialize all handler arrays to NULL
  for (int i = 0; i < 128; i++) {
    cc_handlers[i] = NULL;
    note_on_handlers[i] = NULL;
    note_off_handlers[i] = NULL;
  }

  // MIDIClientRef client;
  MIDIClientCreate(CFSTR("MIDI client"), NULL, NULL, &client);

  MIDIPortRef inputPort;

  MIDIInputPortCreate(client, CFSTR("Input port"), MIDIInputCallback, NULL,
                      &inputPort);

  ItemCount sourceCount = MIDIGetNumberOfSources();
  printf("Found %lu MIDI source%s\n", sourceCount, sourceCount > 1 ? "s" : "");

  for (ItemCount i = 0; i < sourceCount; i++) {
    MIDIEndpointRef source = MIDIGetSource(i);

    CFStringRef nameCF;
    char name[128];

    MIDIPortConnectSource(inputPort, source, NULL);
    OSStatus result =
        MIDIObjectGetStringProperty(source, kMIDIPropertyName, &nameCF);
    if (result == noErr) {

      CFStringGetCString(nameCF, name, sizeof(name), kCFStringEncodingUTF8);
      CFRelease(nameCF);

      printf("MIDI Source %lu: %s\n", (unsigned long)i, name);
    } else {
      printf("MIDI Source %lu: Unable to get name\n", (unsigned long)i);
    }
  }
  init_midi_timing(get_start_time());
}

void send_data(MIDIEndpointRef destination, size_t size, char *data) {

  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *currentPacket = MIDIPacketListInit(packetList);
  for (int i = 0; i < size / 3; i++) {
    Byte midi_data[3] = {data[0], data[1], data[2]};
    currentPacket = MIDIPacketListAdd(packetList, sizeof(buffer), currentPacket,
                                      0, 3, midi_data);
    data += 3;
  }

  MIDISend(output_port, destination, packetList);
}

void midi_out_setup() {
  MIDIOutputPortCreate(client, CFSTR("Output port"), &output_port);
}

int send_note_on(MIDIEndpointRef destination, char channel, char note,
                 char velocity) {

  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *currentPacket = MIDIPacketListInit(packetList);

  Byte midi_data[3];
  midi_data[0] = NOTE_ON | (channel & CHAN_MASK);
  midi_data[1] = note;
  midi_data[2] = velocity;

  currentPacket = MIDIPacketListAdd(packetList, sizeof(buffer), currentPacket,
                                    0, 3, midi_data);

  if (debug) {
    printf("Sending note on: ch=%u note=%u vel=%u\n", channel, note, velocity);
  }

  OSStatus result = MIDISend(output_port, destination, packetList);
  return result == noErr ? 0 : -1;
}

int send_note_off(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                  uint8_t velocity) {

  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[3];
  midi_data[0] = NOTE_OFF | (channel & CHAN_MASK);
  midi_data[1] = note;
  midi_data[2] = velocity;

  printf("send note off %d %d %d\n", channel, note, velocity);
  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 3, midi_data);

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

int send_note_on_ts(MIDIEndpointRef destination, char channel, char note,
                    char velocity, uint64_t ts) {

  MIDITimeStamp t = sample_to_midi_timestamp(ts);

  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *currentPacket = MIDIPacketListInit(packetList);

  Byte midi_data[3];
  midi_data[0] = NOTE_ON | (channel & CHAN_MASK);
  midi_data[1] = note;
  midi_data[2] = velocity;

  currentPacket = MIDIPacketListAdd(packetList, sizeof(buffer), currentPacket,
                                    t, 3, midi_data);

  if (debug) {
    printf("Sending note on: ch=%u note=%u vel=%u %llu\n", channel, note,
           velocity, t);
  }

  OSStatus result = MIDISend(output_port, destination, packetList);
  return result == noErr ? 0 : -1;
}

int send_note_on_dur_ts(MIDIEndpointRef destination, char channel, char note,
                        char velocity, double dur, uint64_t ts) {

  MIDITimeStamp t = sample_to_midi_timestamp(ts - 512);

  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *currentPacket = MIDIPacketListInit(packetList);

  Byte midi_data[3];
  midi_data[0] = NOTE_ON | (channel & CHAN_MASK);
  midi_data[1] = note;
  midi_data[2] = velocity;

  currentPacket = MIDIPacketListAdd(packetList, sizeof(buffer), currentPacket,
                                    t, 3, midi_data);

  if (debug) {
    printf("Sending note on: ch=%u note=%u vel=%u %llu\n", channel, note,
           velocity, t);
  }

  OSStatus result = MIDISend(output_port, destination, packetList);
  return result == noErr ? 0 : -1;
}

int send_note_off_ts(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                     uint8_t velocity, uint64_t ts) {

  MIDITimeStamp t = sample_to_midi_timestamp(ts - 512);
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[3];
  midi_data[0] = NOTE_OFF | (channel & CHAN_MASK);
  midi_data[1] = note;
  midi_data[2] = velocity;

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, t, 3, midi_data);

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

typedef struct _note_data {
  uint8_t channel;
  uint8_t note;
  uint8_t velocity;
} _note_data;

int send_note_ons(MIDIEndpointRef destination, int size, char *note_data_ptr) {
  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packetList);

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel, note, velocity;
    channel = *(note_data_ptr + (i * 3));
    note = *(note_data_ptr + (i * 3) + 1);
    velocity = *(note_data_ptr + (i * 3) + 2);

    Byte midi_data[3];
    midi_data[0] = NOTE_ON | (channel & CHAN_MASK);
    midi_data[1] = note;
    midi_data[2] = velocity;

    if (debug) {
      printf("midi packet midi_data[%d %d %d]\n", midi_data[0], midi_data[1],
             midi_data[2]);
    }

    current_packet = MIDIPacketListAdd(packetList, sizeof(buffer),
                                       current_packet, 0, 3, midi_data);
  }

  OSStatus result = MIDISend(output_port, destination, packetList);
  return result == noErr ? 0 : -1;
}

int send_note_offs(MIDIEndpointRef destination, int size, char *note_data_ptr) {
  Byte buffer[1024];
  MIDIPacketList *packetList = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packetList);

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel, note, velocity;
    channel = *(note_data_ptr + (i * 3));
    note = *(note_data_ptr + (i * 3) + 1);
    velocity = *(note_data_ptr + (i * 3) + 2);

    Byte midi_data[3];
    midi_data[0] = NOTE_OFF | (channel & CHAN_MASK);
    midi_data[1] = note;
    midi_data[2] = velocity;

    current_packet = MIDIPacketListAdd(packetList, sizeof(buffer),
                                       current_packet, 0, 3, midi_data);
  }

  OSStatus result = MIDISend(output_port, destination, packetList);
  return result == noErr ? 0 : -1;
}
// Define Control Change command (0xB0)
#define CONTROL_CHANGE 0xB0

int send_cc(MIDIEndpointRef destination, char channel, char control_number,
            char value) {

  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[3];
  midi_data[0] = CONTROL_CHANGE | (channel & CHAN_MASK);
  midi_data[1] = control_number;
  midi_data[2] = value;

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 3, midi_data);

  if (debug) {
    printf("Sending CC: ch=%u cc=%u val=%u\n", channel, control_number, value);
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

typedef struct _cc_data {
  uint8_t channel;
  uint8_t control_number;
  uint8_t value;
} _cc_data;

int send_ccs(MIDIEndpointRef destination, int size, char *cc_data_ptr) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  for (int i = 0; i < size / 3; i++) {
    uint8_t channel, control_number, value;
    channel = *(cc_data_ptr + (i * 3));
    control_number = *(cc_data_ptr + (i * 3) + 1);
    value = *(cc_data_ptr + (i * 3) + 2);

    Byte midi_data[3];
    midi_data[0] = CONTROL_CHANGE | (channel & CHAN_MASK);
    midi_data[1] = control_number;
    midi_data[2] = value;

    current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                       current_packet, 0, 3, midi_data);
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

MIDIEndpointRef get_destination(ItemCount index) {
  if (index >= MIDIGetNumberOfDestinations()) {
    return 0;
  }
  return MIDIGetDestination(index);
}

MIDIEndpointRef get_destination_by_name(const char *name) {
  ItemCount destCount = MIDIGetNumberOfDestinations();

  for (ItemCount i = 0; i < destCount; i++) {
    MIDIEndpointRef dest = MIDIGetDestination(i);

    CFStringRef nameCF;
    char destName[128];

    OSStatus result =
        MIDIObjectGetStringProperty(dest, kMIDIPropertyName, &nameCF);
    if (result == noErr) {
      CFStringGetCString(nameCF, destName, sizeof(destName),
                         kCFStringEncodingUTF8);
      CFRelease(nameCF);

      if (strcmp(destName, name) == 0) {
        printf("found destination: %s\n", name);
        return dest;
      }
    }
  }
  return 0; // Not found
}

void list_destinations() {
  ItemCount destCount = MIDIGetNumberOfDestinations();
  printf("Found %lu MIDI destination%s\n", destCount, destCount > 1 ? "s" : "");

  for (ItemCount i = 0; i < destCount; i++) {
    MIDIEndpointRef dest = MIDIGetDestination(i);

    CFStringRef nameCF;
    char name[128];

    OSStatus result =
        MIDIObjectGetStringProperty(dest, kMIDIPropertyName, &nameCF);
    if (result == noErr) {
      CFStringGetCString(nameCF, name, sizeof(name), kCFStringEncodingUTF8);
      CFRelease(nameCF);

      printf("MIDI Destination %lu: %s\n", (unsigned long)i, name);
    } else {
      printf("MIDI Destination %lu: Unable to get name\n", (unsigned long)i);
    }
  }
}

// Transport message sending functions
int send_midi_start(MIDIEndpointRef destination) {
  printf("sending midi start\n");
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[1] = {MIDI_START};
  uint64_t t = get_current_sample();

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 1, midi_data);

  if (debug) {
    printf("Sending MIDI start\n");
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  if (result != noErr) {
    printf("Error sending MIDI start: %d\n", (int)result);
  }
  // Send raw MIDI start byte
  // Byte rawStart[1] = {0xFA};
  // result = MIDISendSysex(output_port, rawStart, 1);
  // printf("Raw MIDI start result: %d\n", (int)result);
  return result == noErr ? 0 : -1;
}

int send_midi_stop(MIDIEndpointRef destination) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[1] = {MIDI_STOP};

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 1, midi_data);

  if (debug) {
    printf("Sending MIDI stop\n");
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

int send_midi_continue(MIDIEndpointRef destination) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[1] = {MIDI_CONTINUE};

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 1, midi_data);

  if (debug) {
    printf("Sending MIDI continue\n");
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

int send_midi_clock(MIDIEndpointRef destination) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[1] = {MIDI_CLOCK};

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 1, midi_data);

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

int send_program_change(MIDIEndpointRef destination, uint8_t channel,
                        uint8_t program) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  Byte midi_data[2];
  midi_data[0] = PROGRAM_CHANGE | (channel & CHAN_MASK);
  midi_data[1] = program & 0x7F; // Ensure program is 0-127

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, 0, 2, midi_data);

  if (debug) {
    printf("Sending program change: ch=%u program=%u\n", channel, program);
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}

int send_program_change_ts(MIDIEndpointRef destination, uint8_t channel,
                           uint8_t program, uint64_t ts) {
  Byte buffer[1024];
  MIDIPacketList *packet_list = (MIDIPacketList *)buffer;
  MIDIPacket *current_packet = MIDIPacketListInit(packet_list);

  MIDITimeStamp t = sample_to_midi_timestamp(ts);

  Byte midi_data[2];
  midi_data[0] = PROGRAM_CHANGE | (channel & CHAN_MASK);
  midi_data[1] = program & 0x7F; // Ensure program is 0-127

  current_packet = MIDIPacketListAdd(packet_list, sizeof(buffer),
                                     current_packet, t, 2, midi_data);

  if (debug) {
    printf("Sending program change: ch=%u program=%u\n", channel, program);
  }

  OSStatus result = MIDISend(output_port, destination, packet_list);
  return result == noErr ? 0 : -1;
}
