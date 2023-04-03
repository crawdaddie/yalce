#include "channel.h"
static double out[NUM_BUSES][OUTPUT_CHANNELS * BUF_SIZE] = {0};
static double main_vol = 0.25;

double *channel_out(int chan) { return out[chan]; }

void channel_write(int logical_channel, int physical_channel, int frame,
                   double value) {
  out[logical_channel][frame * OUTPUT_CHANNELS + physical_channel] = value;
}

double channel_read(int logical_channel, int stereo_chan, int frame) {
  return out[logical_channel][frame * OUTPUT_CHANNELS + stereo_chan];
}

double sum_channel_output(int stereo_chan, int frame) {
  double output = 0;
  for (int chan = 0; chan < LAYOUT_CHANNELS; chan++) {
    output += channel_read(chan, stereo_chan, frame);
  }

  return output * main_vol;
}

double sum_channel_output_destructive(int stereo_chan, int frame) {
  double output = 0;
  for (int chan = 0; chan < LAYOUT_CHANNELS; chan++) {
    output += channel_read(chan, stereo_chan, frame);
    channel_write(chan, stereo_chan, frame,
                  0); // set to 0 after writing value out to prevent feeding
                      // back previous block
  }

  return output * main_vol;
}

void zero_channels_after_write(int stereo_chan, int frame) {
  for (int chan = 0; chan < LAYOUT_CHANNELS; chan++) {
    out[chan][frame * OUTPUT_CHANNELS + stereo_chan] = 0;
  }
}

double zero_channel_outputs() {
  double output = 0;

  for (int chan = 0; chan < LAYOUT_CHANNELS; chan++) {
    for (int f = 0; f < BUF_SIZE; f++) {
      for (int s = 0; s < OUTPUT_CHANNELS; s++) {
        out[chan][f * OUTPUT_CHANNELS + s] = 0;
      }
    }
  }

  return output * main_vol;
}

