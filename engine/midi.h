#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <stdint.h>

/* Platform-agnostic MIDI endpoint handle */
typedef struct MIDIEndpoint *MIDIEndpointRef;

/* Callback types */
typedef void (*CCCallback)(double);
typedef void (*NoteCallback)(int, double); /* note number, velocity */

/* Core MIDI setup functions */
void midi_setup(void);
void midi_out_setup(void);

/* Callback registration */
void register_cc_handler(int ch, int cc, CCCallback handler);
void register_note_on_handler(int ch, NoteCallback handler);
void register_note_off_handler(int ch, NoteCallback handler);

/* Debug control */
void toggle_midi_debug(void);

/* MIDI message sending functions */
int send_note_on(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                 uint8_t velocity);

int send_note_off(MIDIEndpointRef destination, uint8_t channel, uint8_t note,
                  uint8_t velocity);

int send_note_ons(MIDIEndpointRef destination, int size,
                  uint8_t *note_data_ptr);
int send_note_offs(MIDIEndpointRef destination, int size,
                   uint8_t *note_data_ptr);

int send_cc(MIDIEndpointRef destination, uint8_t channel,
            uint8_t control_number, uint8_t value);

int send_ccs(MIDIEndpointRef destination, int size, uint8_t *cc_data_ptr);

/* Destination management */
MIDIEndpointRef get_destination(uint32_t index);
MIDIEndpointRef get_destination_by_name(const char *name);
void list_destinations(void);

#endif /* _ENGINE_MIDI_H */
