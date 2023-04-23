#include "midi.h"
#include "ctx.h"
#include "log.h"
#include <CoreMIDI/CoreMIDI.h>

#define CC 0xB0
#define CHAN_MASK 0x0F;

HandlerWrapper Handler[3];

static void handle_cc(MIDIPacket *packet) {
  uint8_t channel = *packet->data & 0x0F;
  uint8_t cc_num = *(packet->data + 1) & 0xFF;
  uint8_t val = *(packet->data + 2);

  /* HandlerWrapper channel_handler = CCHandlers[channel * 129]; */
  /* HandlerWrapper ccnum_handler = CCHandlers[channel * 129 + 1 + cc_num]; */

  /* HandlerWrapper channel_handler = CCHandlers[0]; */
  /* HandlerWrapper ccnum_handler = CCHandler; */

  /* if (channel_handler) { */
  /*   channel_handler(cc_num, *packet->data + 2); */
  /* } */
  /*  */
  /* printf("registered handler %p\n", ccnum_handler.fn_ptr); */
  /* if (ccnum_handler.fn_ptr) { */
  // Handler[0].wrapper(channel, cc_num, *(packet->data + 2),
  //                    Handler[0].registered_name);
  /* } */
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
  write_log("Found %lu MIDI source%s\n", sourceCount,
            sourceCount > 1 ? "s" : "");
  for (int i = 0; i < sourceCount; i++) {
    MIDIEndpointRef source = MIDIGetSource(i);
    MIDIPortConnectSource(inputPort, source, NULL);
    write_log("Connected to MIDI source\n");
  }
}

void register_midi_handler(uint8_t channel, int cc_num, CCHandler handler) {
  uint8_t cc_offset = cc_num == -1 ? 0 : 1 + cc_num;
  printf("%p handler\n", handler);
  /* CCHandlers[channel * 129 + cc_offset] = handler; */
}
