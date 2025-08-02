#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <stddef.h>
#include <stdint.h>

// Cross-platform types
#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
typedef MIDIEndpointRef midi_endpoint_t;
typedef ItemCount midi_count_t;
#else
// Linux/ALSA types
typedef void *midi_endpoint_t;
typedef unsigned int midi_count_t;
#endif

// Callback type definitions
typedef void (*CCCallback)(double);
typedef void (*NoteCallback)(int, double); // note number, velocity
typedef void (*ProgramChangeCallback)(int channel, int program);
typedef void (*TransportCallback)(void);

// Core MIDI setup functions
void midi_setup();
void midi_out_setup();

// Handler registration functions
void register_cc_handler(int ch, int cc, CCCallback handler);
void register_note_on_handler(int ch, NoteCallback handler);
void register_note_off_handler(int ch, NoteCallback handler);
void register_program_change_handler(int ch, ProgramChangeCallback handler);
void register_midi_clock_handler(TransportCallback handler);
void register_midi_start_handler(TransportCallback handler);
void register_midi_continue_handler(TransportCallback handler);
void register_midi_stop_handler(TransportCallback handler);

// Debug functions
void toggle_midi_debug();

// MIDI output functions
int send_note_on(midi_endpoint_t destination, char channel, char note,
                 char velocity);
int send_note_off(midi_endpoint_t destination, uint8_t channel, uint8_t note,
                  uint8_t velocity);
int send_note_on_ts(midi_endpoint_t destination, char channel, char note,
                    char velocity, uint64_t ts);
int send_note_off_ts(midi_endpoint_t destination, uint8_t channel, uint8_t note,
                     uint8_t velocity, uint64_t ts);
int send_note_on_dur_ts(midi_endpoint_t destination, char channel, char note,
                        char velocity, double dur, uint64_t ts);

// Bulk MIDI functions
int send_note_ons(midi_endpoint_t destination, int size, char *note_data_ptr);
int send_note_offs(midi_endpoint_t destination, int size, char *note_data_ptr);

// Control Change functions
int send_cc(midi_endpoint_t destination, char channel, char control_number,
            char value);
int send_ccs(midi_endpoint_t destination, int size, char *cc_data_ptr);

// Program Change functions
int send_program_change(midi_endpoint_t destination, uint8_t channel,
                        uint8_t program);
int send_program_change_ts(midi_endpoint_t destination, uint8_t channel,
                           uint8_t program, uint64_t ts);

// Transport functions
int send_midi_start(midi_endpoint_t destination);
int send_midi_stop(midi_endpoint_t destination);
int send_midi_continue(midi_endpoint_t destination);
int send_midi_clock(midi_endpoint_t destination);

// Device management
midi_endpoint_t get_destination(midi_count_t index);
midi_endpoint_t get_destination_by_name(const char *name);
void list_destinations();

// Raw data sending
void send_data(midi_endpoint_t destination, size_t size, char *data);

#endif
