#ifndef _MIDI_H
#define _MIDI_H
#include <stdint.h>

typedef void (*CCHandler)(uint8_t chan, uint8_t cc, uint8_t val,
                          const char *registered_name);

typedef struct {
  CCHandler wrapper;
  const char *registered_name;
} HandlerWrapper;

extern HandlerWrapper Handler[3];
void midi_setup();
void register_midi_handler(uint8_t channel, int cc_num, CCHandler handler);
#endif
