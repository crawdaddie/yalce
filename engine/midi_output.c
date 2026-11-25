#include "midi.h"
#include "scheduling.h"
#include <math.h>

// Integer arguments keep MIDI bytes out of pattern code.
enum { MIDI_CHANNEL_MAX = 15, MIDI_DATA_MAX = 127 };

int midi_open(const char *name) {
  midi_out_setup();
  midi_endpoint_t destination = get_destination_by_name(name);
  if (!destination || midi_connect_output(destination) < 0) {
    return 0;
  }
  return (int)(uintptr_t)destination;
}

int midi_note_at(int destination, int channel, double pitch, int velocity,
                 double duration, uint64_t tick) {
  if (destination <= 0 || channel < 0 || channel > MIDI_CHANNEL_MAX ||
      !isfinite(pitch) || pitch < 0. || pitch > MIDI_DATA_MAX ||
      floor(pitch) != pitch || velocity < 1 || velocity > MIDI_DATA_MAX ||
      !isfinite(duration) || duration <= 0.) {
    return -1;
  }
  return send_note_on_dur_ts((midi_endpoint_t)(uintptr_t)destination, channel,
                             (int)pitch, velocity, duration, tick);
}

int midi_note(int destination, int channel, double pitch, int velocity,
              double duration) {
  return midi_note_at(destination, channel, pitch, velocity, duration,
                       get_tl_tick());
}

int midi_panic(int destination, int channel) {
  if (destination <= 0 || channel < 0 || channel > MIDI_CHANNEL_MAX) {
    return -1;
  }
  enum { ALL_SOUND_OFF = 120 };
  return send_cc((midi_endpoint_t)(uintptr_t)destination, channel, ALL_SOUND_OFF, 0);
}
