#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <stdint.h>

typedef void (*CCCallback)(double);
typedef void (*NoteCallback)(int, double); // note number, velocity

void midi_setup();

void register_cc_handler(CCCallback handler, int ch, int cc);
void register_note_on_handler(NoteCallback handler, int ch);
void register_note_off_handler(NoteCallback handler, int ch);

void toggle_midi_debug();

#endif
