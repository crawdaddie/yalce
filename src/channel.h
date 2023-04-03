#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "common.h"

double *channel_out(int chan);

void channel_write(int logical_channel, int physical_channel, int frame,
                   double value);

double channel_read(int logical_channel, int stereo_chan, int frame);

double sum_channel_output(int stereo_chan, int frame);

double sum_channel_output_destructive(int stereo_chan, int frame);

void zero_channels_after_write(int stereo_chan, int frame);

double zero_channel_outputs();


#endif
