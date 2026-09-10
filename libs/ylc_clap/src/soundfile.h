#ifndef YLC_CLAP_SOUNDFILE_H
#define YLC_CLAP_SOUNDFILE_H

#include "ylc_datatypes.h"

#include <stdint.h>
#include <stdbool.h>

#define YLC_SOUNDFILE_MAX_SLOTS 64u
#define YLC_SOUNDFILE_UI_MAX_SLOTS 16u
#define YLC_SOUNDFILE_PATH_SIZE 1024

typedef struct ylc_plugin ylc_plugin_t;

typedef struct ylc_soundfile {
  char path[YLC_SOUNDFILE_PATH_SIZE];
  char user_path[YLC_SOUNDFILE_PATH_SIZE];
  double *data;
  uint64_t frames;
  int channels;
  int samplerate;
  uint64_t region_start;
  uint64_t region_end;
  bool loaded;
} ylc_soundfile_t;

typedef struct ylc_soundfile_slot {
  uint64_t key;
  ylc_soundfile_t *handle;
} ylc_soundfile_slot_t;

typedef struct ylc_soundfile_inherit {
  char path[YLC_SOUNDFILE_PATH_SIZE];
  uint64_t region_start;
  uint64_t region_end;
} ylc_soundfile_inherit_t;

void *ylc_plugin_soundfile_ui(uint64_t key, _String default_path);
int ylc_plugin_sf_channels(void *handle);
int ylc_plugin_sf_samplerate(void *handle);
_DoubleArray ylc_plugin_sf_data(void *handle);

ylc_soundfile_slot_t *ylc_soundfile_find(ylc_plugin_t *self, uint64_t key);
ylc_soundfile_slot_t *ylc_soundfile_create(ylc_plugin_t *self, uint64_t key);
bool ylc_soundfile_load(ylc_soundfile_t *sf);
void ylc_soundfile_free_all(ylc_plugin_t *self);
void ylc_soundfile_set_dropped_path(ylc_soundfile_t *sf, const char *uri);
void ylc_soundfile_cache_clear(void);

#endif
