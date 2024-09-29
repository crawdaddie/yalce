#ifndef _ENGINE_MIDI_H
#define _ENGINE_MIDI_H

#include <stdint.h>

typedef void (*CCCallback)(int);

void midi_setup();

void register_cc_handler(CCCallback handler, int ch, int cc);

#endif
