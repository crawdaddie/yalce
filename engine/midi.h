#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <CoreMIDI/CoreMIDI.h>
#include <stdint.h>

typedef void (*CCCallback)(double);
typedef void (*NoteCallback)(int, double); // note number, velocity

void midi_setup();

void register_cc_handler(int ch, int cc, CCCallback handler);
void register_note_on_handler(int ch, NoteCallback handler);
void register_note_off_handler(int ch, NoteCallback handler);

void toggle_midi_debug();

int send_note_on(MIDIEndpointRef destination, char channel, char note,
                 char velocity);

int send_note_off(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                  uint8_t velocity);

int send_note_ons(MIDIEndpointRef destination, int size, char *note_data_ptr);
int send_note_offs(MIDIEndpointRef destination, int size, char *note_data_ptr);

MIDIEndpointRef get_destination(ItemCount index);
MIDIEndpointRef get_destination_by_name(const char *name);
void list_destinations();

void midi_out_setup();
#endif
