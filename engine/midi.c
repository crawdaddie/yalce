#include "midi.h"

#include <CoreMIDI/CoreMIDI.h>

#define CC 0xB0
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define CHAN_MASK 0x0F

#define REC_127 0.007874015748031496

// Arrays to store handlers
static CCCallback cc_handlers[128];
static NoteCallback note_on_handlers[128];
static NoteCallback note_off_handlers[128];

// Register handler functions
void register_cc_handler(CCCallback handler, int ch, int cc) {
  cc_handlers[cc] = handler;
}

void register_note_on_handler(NoteCallback handler, int ch) {
  note_on_handlers[ch] = handler;
}

void register_note_off_handler(NoteCallback handler, int ch) {
  note_off_handlers[ch] = handler;
}

// Handler implementations
static void handle_cc(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t cc = *(packet->data + 1) & 0xFF;
  uint8_t val = *(packet->data + 2);
  CCCallback handler = cc_handlers[cc];
  if (handler != NULL) {
    handler((double)(val * REC_127));
  }

  // printf("%d %d %d\n", ch, cc, val);
}

static void handle_note_on(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t note = *(packet->data + 1) & 0xFF;
  uint8_t velocity = *(packet->data + 2);
  NoteCallback handler = note_on_handlers[ch];
  if (handler != NULL) {
    handler(note, (double)(velocity * REC_127));
  }
}

static void handle_note_off(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t note = *(packet->data + 1) & 0xFF;
  uint8_t velocity = *(packet->data + 2);
  NoteCallback handler = note_off_handlers[ch];
  if (handler != NULL) {
    handler(note, (double)(velocity * REC_127));
  }

  // printf("Note Off: %d %d %d\n", ch, note, velocity);
}

static void MIDIInputCallback(const MIDIPacketList *pktlist,
                              void *readProcRefCon, void *srcConnRefCon) {

  MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
  for (int i = 0; i < pktlist->numPackets; i++) {
    switch (*packet->data & 0xF0) {
    case CC:
      handle_cc(packet);
      break;
    case NOTE_ON:
      handle_note_on(packet);
      break;
    case NOTE_OFF:
      handle_note_off(packet);
      break;
    default:
      break;
    }
    packet = MIDIPacketNext(packet);
  }
}

void midi_setup() {
  // Initialize all handler arrays to NULL
  for (int i = 0; i < 128; i++) {
    cc_handlers[i] = NULL;
    note_on_handlers[i] = NULL;
    note_off_handlers[i] = NULL;
  }

  MIDIClientRef client;
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
}
