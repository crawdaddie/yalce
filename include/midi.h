#ifndef _MIDI_H
#define _MIDI_H
#include <stdint.h>

typedef int (*CCHandler)(int channel, int cc, int val);
typedef struct {
  CCHandler handler;
  const char *registered_name;
} HandlerWrapper;

extern HandlerWrapper Handler[3];

void midi_setup();
void register_midi_handler(uint8_t channel, int cc_num,
                           int (*CCHandler)(int channel, int cc, int val));
#endif
