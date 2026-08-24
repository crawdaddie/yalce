#include "plugin_internal.h"
#include "debug.h"
#include "soundfile.h"

#include <sndfile.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct ylc_soundfile_cache_entry {
  char path[YLC_SOUNDFILE_PATH_SIZE];
  double *data;
  uint64_t frames;
  int channels;
  int samplerate;
  struct ylc_soundfile_cache_entry *next;
} ylc_soundfile_cache_entry_t;

static ylc_soundfile_cache_entry_t *ylc_soundfile_cache = NULL;
static pthread_mutex_t ylc_soundfile_cache_lock = PTHREAD_MUTEX_INITIALIZER;

static ylc_soundfile_cache_entry_t *
ylc_soundfile_cache_lookup_locked(const char *path) {
  for (ylc_soundfile_cache_entry_t *e = ylc_soundfile_cache; e; e = e->next) {
    if (strcmp(e->path, path) == 0) {
      return e;
    }
  }
  return NULL;
}

static ylc_soundfile_cache_entry_t *
ylc_soundfile_cache_insert_locked(const char *path, double *data,
                                   uint64_t frames, int channels,
                                   int samplerate) {
  ylc_soundfile_cache_entry_t *e =
      (ylc_soundfile_cache_entry_t *)calloc(1, sizeof(*e));
  if (!e) {
    return NULL;
  }
  snprintf(e->path, sizeof(e->path), "%s", path);
  e->data = data;
  e->frames = frames;
  e->channels = channels;
  e->samplerate = samplerate;
  e->next = ylc_soundfile_cache;
  ylc_soundfile_cache = e;
  return e;
}

void ylc_soundfile_cache_clear(void) {
  pthread_mutex_lock(&ylc_soundfile_cache_lock);
  ylc_soundfile_cache_entry_t *e = ylc_soundfile_cache;
  while (e) {
    ylc_soundfile_cache_entry_t *next = e->next;
    free(e->data);
    free(e);
    e = next;
  }
  ylc_soundfile_cache = NULL;
  pthread_mutex_unlock(&ylc_soundfile_cache_lock);
}

static ylc_soundfile_t *ylc_soundfile_handle_alloc(void) {
  ylc_soundfile_t *sf = (ylc_soundfile_t *)calloc(1, sizeof(*sf));
  return sf;
}

ylc_soundfile_slot_t *ylc_soundfile_find(ylc_plugin_t *self, uint64_t key) {
  if (!self) {
    return NULL;
  }
  for (uint32_t i = 0; i < self->soundfile_count; ++i) {
    if (self->soundfiles[i].key == key) {
      return &self->soundfiles[i];
    }
  }
  return NULL;
}

ylc_soundfile_slot_t *ylc_soundfile_create(ylc_plugin_t *self, uint64_t key) {
  if (!self || self->soundfile_count >= YLC_SOUNDFILE_MAX_SLOTS) {
    return NULL;
  }
  if (self->soundfile_count >= self->soundfile_capacity) {
    uint32_t next_cap = self->soundfile_capacity > 0
                            ? self->soundfile_capacity * 2
                            : 8;
    if (next_cap > YLC_SOUNDFILE_MAX_SLOTS) {
      next_cap = YLC_SOUNDFILE_MAX_SLOTS;
    }
    ylc_soundfile_slot_t *next = (ylc_soundfile_slot_t *)realloc(
        self->soundfiles, sizeof(*next) * next_cap);
    if (!next) {
      return NULL;
    }
    self->soundfiles = next;
    self->soundfile_capacity = next_cap;
  }
  ylc_soundfile_slot_t *slot = &self->soundfiles[self->soundfile_count++];
  memset(slot, 0, sizeof(*slot));
  slot->key = key;
  slot->handle = ylc_soundfile_handle_alloc();
  return slot;
}

bool ylc_soundfile_load(ylc_soundfile_t *sf) {
  if (!sf || sf->path[0] == '\0') {
    return false;
  }

  pthread_mutex_lock(&ylc_soundfile_cache_lock);
  ylc_soundfile_cache_entry_t *entry =
      ylc_soundfile_cache_lookup_locked(sf->path);
  if (entry) {
    sf->data = entry->data;
    sf->frames = entry->frames;
    sf->channels = entry->channels;
    sf->samplerate = entry->samplerate;
    if (!sf->loaded) {
      sf->region_start = 0;
      sf->region_end = entry->frames;
    }
    sf->loaded = true;
    pthread_mutex_unlock(&ylc_soundfile_cache_lock);
    return true;
  }
  pthread_mutex_unlock(&ylc_soundfile_cache_lock);

  const char *path = sf->path;
  char *full_path = NULL;

  if (path[0] == '~') {
    const char *home = getenv("HOME");
    if (!home) {
      return false;
    }
    size_t need = strlen(home) + strlen(path);
    full_path = (char *)calloc(1, need);
    if (!full_path) {
      return false;
    }
    snprintf(full_path, need, "%s%s", home, path + 1);
    path = full_path;
  }

  SF_INFO sfinfo;
  memset(&sfinfo, 0, sizeof(sfinfo));
  SNDFILE *infile = sf_open(path, SFM_READ, &sfinfo);
  if (!infile) {
    free(full_path);
    sf->loaded = false;
    return false;
  }

  uint64_t total = (uint64_t)sfinfo.frames * (uint64_t)sfinfo.channels;
  double *buf = NULL;
  if (total > 0) {
    buf = (double *)calloc((size_t)total, sizeof(double));
    if (!buf) {
      sf_close(infile);
      free(full_path);
      return false;
    }
    sf_read_double(infile, buf, (sf_count_t)total);
  }
  sf_close(infile);
  free(full_path);

  pthread_mutex_lock(&ylc_soundfile_cache_lock);
  entry = ylc_soundfile_cache_lookup_locked(sf->path);
  if (entry) {
    free(buf);
    sf->data = entry->data;
    sf->frames = entry->frames;
    sf->channels = entry->channels;
    sf->samplerate = entry->samplerate;
  } else {
    entry = ylc_soundfile_cache_insert_locked(sf->path, buf,
                                               (uint64_t)sfinfo.frames,
                                               sfinfo.channels,
                                               sfinfo.samplerate);
    if (!entry) {
      free(buf);
      pthread_mutex_unlock(&ylc_soundfile_cache_lock);
      return false;
    }
    sf->data = entry->data;
    sf->frames = entry->frames;
    sf->channels = entry->channels;
    sf->samplerate = entry->samplerate;
  }
  if (!sf->loaded) {
    sf->region_start = 0;
    sf->region_end = sf->frames;
  }
  sf->loaded = true;
  pthread_mutex_unlock(&ylc_soundfile_cache_lock);
  return true;
}

void ylc_soundfile_free_all(ylc_plugin_t *self) {
  if (!self) {
    return;
  }
  for (uint32_t i = 0; i < self->soundfile_count; ++i) {
    if (self->soundfiles[i].handle) {
      free(self->soundfiles[i].handle);
    }
  }
  free(self->soundfiles);
  self->soundfiles = NULL;
  self->soundfile_count = 0;
  self->soundfile_capacity = 0;
}

static void ylc_soundfile_uri_to_path(const char *uri, char *out,
                                      size_t out_size) {
  if (!uri || !out || out_size == 0) {
    return;
  }
  const char *file_scheme = "file://";
  size_t scheme_len = strlen(file_scheme);
  if (strncmp(uri, file_scheme, scheme_len) == 0) {
    const char *p = uri + scheme_len;
    if (p[0] == '/' && p[1] == '/') {
      p++;
    }
    snprintf(out, out_size, "%s", p);
  } else {
    snprintf(out, out_size, "%s", uri);
  }
}

void ylc_soundfile_set_dropped_path(ylc_soundfile_t *sf, const char *uri) {
  if (!sf || !uri) {
    return;
  }
  char path[YLC_SOUNDFILE_PATH_SIZE] = {0};
  ylc_soundfile_uri_to_path(uri, path, sizeof(path));
  if (path[0] == '\0') {
    return;
  }
  snprintf(sf->user_path, sizeof(sf->user_path), "%s", path);
  snprintf(sf->path, sizeof(sf->path), "%s", path);
  sf->region_start = 0;
  sf->region_end = 0;
  ylc_soundfile_load(sf);
  if (sf->loaded && sf->region_end == 0) {
    sf->region_end = sf->frames;
  }
}

void *ylc_plugin_soundfile_ui(uint64_t key, _String default_path) {
  ylc_plugin_t *self = ylc_debug_printf_context;
  if (!self) {
    return NULL;
  }

  const uint32_t inherit_idx = self->sf_inherit_index++;

  ylc_soundfile_slot_t *slot = ylc_soundfile_find(self, key);
  if (!slot) {
    slot = ylc_soundfile_create(self, key);
  }
  if (!slot || !slot->handle) {
    return NULL;
  }

  if (inherit_idx < self->sf_inherit_count && !slot->handle->loaded) {
    snprintf(slot->handle->user_path, sizeof(slot->handle->user_path),
             "%s", self->sf_inherit[inherit_idx].path);
  }

  char new_path[YLC_SOUNDFILE_PATH_SIZE] = {0};
  if (slot->handle->user_path[0] != '\0') {
    snprintf(new_path, sizeof(new_path), "%s", slot->handle->user_path);
  } else {
    int32_t len = default_path.size > 0 ? default_path.size : 0;
    if (len < 0) {
      len = 0;
    }
    if ((size_t)len >= sizeof(new_path)) {
      len = (int32_t)sizeof(new_path) - 1;
    }
    if (len > 0 && default_path.chars) {
      memcpy(new_path, default_path.chars, (size_t)len);
    }
    new_path[len] = '\0';
  }

  if (!slot->handle->loaded || strcmp(slot->handle->path, new_path) != 0) {
    memcpy(slot->handle->path, new_path, strlen(new_path) + 1);
    ylc_soundfile_load(slot->handle);
    ylc_debug_log(self, "SoundFileUI loaded: %s (frames=%lu ch=%d sr=%d)",
                  slot->handle->path, (unsigned long)slot->handle->frames,
                  slot->handle->channels, slot->handle->samplerate);
    if (inherit_idx < self->sf_inherit_count) {
      uint64_t rs = self->sf_inherit[inherit_idx].region_start;
      uint64_t re = self->sf_inherit[inherit_idx].region_end;
      if (rs < slot->handle->frames) {
        slot->handle->region_start = rs;
      }
      if (re > 0 && re <= slot->handle->frames && re > rs) {
        slot->handle->region_end = re;
      }
    }
  }

  for (uint32_t i = 0; i < self->ui_count; ++i) {
    if (self->ui_slots[i].kind == YLC_UI_SOUNDFILE &&
        self->ui_slots[i].soundfile == slot->handle) {
      return slot->handle;
    }
  }
  if (self->ui_count < YLC_UI_MAX_SLOTS) {
    self->ui_slots[self->ui_count++] = (ylc_ui_slot_t){
        .kind = YLC_UI_SOUNDFILE, .soundfile = slot->handle};
  }

  return slot->handle;
}

int ylc_plugin_sf_channels(void *handle) {
  ylc_soundfile_t *sf = (ylc_soundfile_t *)handle;
  return sf ? sf->channels : 0;
}

int ylc_plugin_sf_samplerate(void *handle) {
  ylc_soundfile_t *sf = (ylc_soundfile_t *)handle;
  return sf ? sf->samplerate : 0;
}

_DoubleArray ylc_plugin_sf_data(void *handle) {
  ylc_soundfile_t *sf = (ylc_soundfile_t *)handle;
  if (!sf || !sf->data || sf->frames == 0) {
    return (_DoubleArray){.size = 0, .offset = 0, .data = NULL};
  }
  uint64_t start = sf->region_start;
  uint64_t end = sf->region_end;
  if (start >= sf->frames) {
    start = 0;
  }
  if (end > sf->frames || end < start) {
    end = sf->frames;
  }
  uint64_t region_frames = end - start;
  uint64_t total_samples = region_frames * (uint64_t)sf->channels;
  if (total_samples > (uint64_t)INT32_MAX) {
    total_samples = (uint64_t)INT32_MAX;
  }
  return (_DoubleArray){
      .size = (int32_t)total_samples,
      .offset = 0,
      .data = sf->data + (start * (uint64_t)sf->channels),
  };
}
