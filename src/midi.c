#include "midi.h"
#include "log.h"
#include <CoreMIDI/CoreMIDI.h>

static void MyMIDIInputCallback(const MIDIPacketList *pktlist,
                                void *readProcRefCon, void *srcConnRefCon) {
  MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
  for (int i = 0; i < pktlist->numPackets; i++) {
    for (int j = 0; j < packet->length; j++) {
      write_log("Received MIDI message: 0x%x\n", packet->data[j]);
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

  MIDIInputPortCreate(client, CFSTR("Input port"), MyMIDIInputCallback, NULL,
                      &inputPort);

  // get the list of MIDI sources
  ItemCount sourceCount = MIDIGetNumberOfSources();
  write_log("Found %lu MIDI sources\n", sourceCount);

  // connect to the first source
  MIDIEndpointRef source = MIDIGetSource(0);
  MIDIPortConnectSource(inputPort, source, NULL);
  printf("Connected to MIDI source\n");
}
