#include "midi.h"

#include <CoreMIDI/CoreMIDI.h>

#define CC 0xB0
#define CHAN_MASK 0x0F

#define REC_127 0.007874015748031496
// Define the function pointer type
typedef void (*CCCallback)(double);
static CCCallback cc_handlers[128];

void register_cc_handler(CCCallback handler, int ch, int cc) {
  cc_handlers[cc] = handler;
}

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

static void MIDIInputCallback(const MIDIPacketList *pktlist,
                              void *readProcRefCon, void *srcConnRefCon) {
  MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
  for (int i = 0; i < pktlist->numPackets; i++) {
    switch (*packet->data & 0xF0) {
    case CC: {
      handle_cc(packet);
      break;
    }
    default:
      break;
    }
    packet = MIDIPacketNext(packet);
  }
}

void midi_setup() {
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
