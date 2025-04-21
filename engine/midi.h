#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <CoreMIDI/CoreMIDI.h>
#include <stdint.h>

typedef void (*CCCallback)(double);
typedef void (*NoteCallback)(int, double); // note number, velocity

void midi_setup();

void register_cc_handler(CCCallback handler, int ch, int cc);
void register_note_on_handler(NoteCallback handler, int ch);
void register_note_off_handler(NoteCallback handler, int ch);

void toggle_midi_debug();

int send_note_on(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                 uint8_t velocity);

int send_note_off(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                  uint8_t velocity);

MIDIEndpointRef get_destination(ItemCount index);
MIDIEndpointRef get_destination_by_name(const char *name);
void list_destinations();

void midi_out_setup();
#endif
