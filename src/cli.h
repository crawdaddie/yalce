#ifndef TERM_H
#define TERM_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <soundio/soundio.h>
#include <string.h>

#include <stdint.h>
#include <stdlib.h>

#include <math.h>

void enableRawMode();
void disableRawMode();

static int ss_usage(char *exe);

int ss_setup(int argc, char **argv, char *stream_name, char *device_id,
             enum SoundIoBackend *backend);

#endif
