#ifndef YLC_FILE_WATCH_H
#define YLC_FILE_WATCH_H

#include <stdint.h>
#include <stdio.h>

#include "ylc_datatypes.h"

typedef struct FileWatcher FileWatcher;

/* Matches Yalce's flattened String callback ABI. */
typedef void (*FileChangeCallback)(int32_t size, int32_t offset,
                                   const char *chars, void *userdata);

FileWatcher *file_watch_start(void);

int file_watch_register(FileWatcher *watcher, FILE *file,
                        FileChangeCallback callback, void *userdata);

void file_watch_dispatch(FileWatcher *watcher);

void file_watch_unregister(FileWatcher *watcher, FILE *file);

void file_watch_stop(FileWatcher *watcher);

#endif
