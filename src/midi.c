#include "midi.h"
#include "ctx.h"
#include "log.h"
#include <CoreMIDI/CoreMIDI.h>

#define CC 0xB0
#define CHAN_MASK 0x0F;

HandlerWrapper Handler[3];

static void handle_cc(MIDIPacket *packet) {
  uint8_t ch = *packet->data & 0x0F;
  uint8_t cc = *(packet->data + 1) & 0xFF;
  uint8_t val = *(packet->data + 2);

  printf("MIDI cc: %d %d %d\n", ch, cc, val);
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
  // create a client
  MIDIClientRef client;
  MIDIClientCreate(CFSTR("MIDI client"), NULL, NULL, &client);

  // create an input port
  MIDIPortRef inputPort;

  MIDIInputPortCreate(client, CFSTR("Input port"), MIDIInputCallback, NULL,
                      &inputPort);

  // get the list of MIDI sources
  ItemCount sourceCount = MIDIGetNumberOfSources();
  printf("num sources %lu\n", sourceCount);
  for (int i = 0; i < sourceCount; i++) {
    MIDIEndpointRef source = MIDIGetSource(i);
    MIDIPortConnectSource(inputPort, source, NULL);
  }
}

void register_midi_handler(uint8_t channel, int cc_num,
                           int (*CCHandler)(int channel, int cc, int val)) {
  printf("register\n");
  printf("%p handler %d %d\n", CCHandler, channel, cc_num);
  Handler[0].handler = CCHandler;
}
