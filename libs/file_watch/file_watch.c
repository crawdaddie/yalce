#define _GNU_SOURCE

#include "file_watch.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>

enum {
  STOP_READ = 0,
  STOP_WRITE = 1,
  INOTIFY_INDEX = 0,
  STOP_INDEX = 1,
  EVENT_BUFFER_SIZE = 8192,
  PATH_BUFFER_SIZE = 4096,
  FILE_CHANGE_DEBOUNCE_NS = 100000000,
};

struct FileWatchEntry {
  FILE *file;
  FileChangeCallback callback;
  void *userdata;
  int watch_descriptor;
  char *directory;
  char *name;
  struct timespec last_callback;
  bool has_callback;
  bool pending_callback;
  struct FileWatchEntry *next;
};

struct FileWatcher {
  pthread_t thread;
  pthread_mutex_t mutex;
  atomic_bool stopped;
  int inotify_fd;
  int stop_pipe[2];
  struct FileWatchEntry *entries;
};

struct PendingCallback {
  FILE *file;
  FileChangeCallback callback;
  void *userdata;
};

static _String read_content(FILE *file) {
  _String empty = {0, 0, NULL};
  flockfile(file);

  long original_position = ftell(file);
  if (original_position < 0 || fseek(file, 0, SEEK_END) != 0) {
    funlockfile(file);
    return empty;
  }

  long file_size = ftell(file);
  if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0 || file_size > INT32_MAX) {
    fseek(file, original_position, SEEK_SET);
    funlockfile(file);
    return empty;
  }

  char *content = malloc((size_t)file_size + 1);
  if (!content) {
    fseek(file, original_position, SEEK_SET);
    funlockfile(file);
    return empty;
  }

  size_t bytes_read = fread(content, 1, (size_t)file_size, file);
  content[bytes_read] = '\0';
  fseek(file, original_position, SEEK_SET);
  funlockfile(file);

  return (_String){(int32_t)bytes_read, 0, content};
}

static void invoke_callback(FILE *file, FileChangeCallback callback,
                            void *userdata) {
  _String content = read_content(file);
  if (!content.chars) {
    return;
  }

  callback(content.size, content.offset, content.chars, userdata);
}

static char *copy_string(const char *source, size_t length) {
  char *result = malloc(length + 1);
  if (!result) {
    return NULL;
  }

  memcpy(result, source, length);
  result[length] = '\0';
  return result;
}

static bool split_path(const char *path, char **directory, char **name) {
  const char *slash = strrchr(path, '/');

  if (!slash) {
    *directory = copy_string(".", 1);
    *name = copy_string(path, strlen(path));
  } else if (slash == path) {
    *directory = copy_string("/", 1);
    *name = copy_string(slash + 1, strlen(slash + 1));
  } else {
    *directory = copy_string(path, (size_t)(slash - path));
    *name = copy_string(slash + 1, strlen(slash + 1));
  }

  if (!*directory || !*name || (*name)[0] == '\0') {
    free(*directory);
    free(*name);
    *directory = NULL;
    *name = NULL;
    return false;
  }

  return true;
}

static bool file_path(FILE *file, char **directory, char **name) {
  char link_path[PATH_BUFFER_SIZE];
  char target[PATH_BUFFER_SIZE];
  int length =
      snprintf(link_path, sizeof(link_path), "/proc/self/fd/%d", fileno(file));
  if (length < 0 || (size_t)length >= sizeof(link_path)) {
    return false;
  }

  ssize_t target_length = readlink(link_path, target, sizeof(target) - 1);
  if (target_length < 0) {
    return false;
  }

  target[target_length] = '\0';
  return split_path(target, directory, name);
}

static void wake_thread(FileWatcher *watcher) {
  const char byte = 1;
  ssize_t ignored = write(watcher->stop_pipe[STOP_WRITE], &byte, 1);
  (void)ignored;
}

static void drain_pipe(int fd) {
  char buffer[64];
  ssize_t ignored = read(fd, buffer, sizeof(buffer));
  (void)ignored;
}

static void mark_callbacks(FileWatcher *watcher, int watch_descriptor,
                           const char *name) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  for (struct FileWatchEntry *entry = watcher->entries; entry;
       entry = entry->next) {
    if (entry->watch_descriptor != watch_descriptor ||
        strcmp(entry->name, name) != 0) {
      continue;
    }

    int64_t elapsed =
        (int64_t)(now.tv_sec - entry->last_callback.tv_sec) * 1000000000LL +
        now.tv_nsec - entry->last_callback.tv_nsec;
    if (entry->pending_callback ||
        (entry->has_callback && elapsed < FILE_CHANGE_DEBOUNCE_NS)) {
      continue;
    }

    entry->last_callback = now;
    entry->has_callback = true;
    entry->pending_callback = true;
  }
}

static void dispatch_event(FileWatcher *watcher,
                           const struct inotify_event *event) {
  if (event->len == 0) {
    return;
  }

  pthread_mutex_lock(&watcher->mutex);
  mark_callbacks(watcher, event->wd, event->name);
  pthread_mutex_unlock(&watcher->mutex);
}

void file_watch_dispatch(FileWatcher *watcher) {
  if (!watcher || atomic_load(&watcher->stopped)) {
    return;
  }

  pthread_mutex_lock(&watcher->mutex);
  size_t count = 0;
  for (struct FileWatchEntry *entry = watcher->entries; entry;
       entry = entry->next) {
    if (entry->pending_callback) {
      count++;
    }
  }

  struct PendingCallback *callbacks = calloc(count, sizeof(*callbacks));
  if (!callbacks) {
    pthread_mutex_unlock(&watcher->mutex);
    return;
  }

  size_t index = 0;
  for (struct FileWatchEntry *entry = watcher->entries; entry;
       entry = entry->next) {
    if (!entry->pending_callback) {
      continue;
    }

    entry->pending_callback = false;
    callbacks[index++] = (struct PendingCallback){
        .file = entry->file,
        .callback = entry->callback,
        .userdata = entry->userdata,
    };
  }

  pthread_mutex_unlock(&watcher->mutex);

  for (size_t index = 0; index < count; index++) {
    invoke_callback(callbacks[index].file, callbacks[index].callback,
                    callbacks[index].userdata);
  }

  free(callbacks);
}

static void drain_inotify(FileWatcher *watcher) {
  char buffer[EVENT_BUFFER_SIZE];

  for (;;) {
    ssize_t size = read(watcher->inotify_fd, buffer, sizeof(buffer));
    if (size < 0) {
      if (errno == EINTR) {
        continue;
      }
      return;
    }

    if (size == 0) {
      return;
    }

    for (char *cursor = buffer; cursor < buffer + size;) {
      struct inotify_event *event = (struct inotify_event *)cursor;
      const uint32_t changes = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE |
                               IN_ATTRIB;

      if ((event->mask & changes) != 0) {
        dispatch_event(watcher, event);
      }

      cursor += sizeof(*event) + event->len;
    }
  }
}

static void *watch_thread(void *arg) {
  FileWatcher *watcher = arg;
  struct pollfd fds[2] = {
      {.fd = watcher->inotify_fd, .events = POLLIN},
      {.fd = watcher->stop_pipe[STOP_READ], .events = POLLIN},
  };

  while (!atomic_load(&watcher->stopped)) {
    int result = poll(fds, 2, -1);
    if (result < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    if (fds[STOP_INDEX].revents != 0) {
      drain_pipe(watcher->stop_pipe[STOP_READ]);
    }

    if (fds[INOTIFY_INDEX].revents & POLLIN) {
      drain_inotify(watcher);
    }
  }

  return NULL;
}

FileWatcher *file_watch_start(void) {
  FileWatcher *watcher = calloc(1, sizeof(*watcher));
  if (!watcher) {
    return NULL;
  }

  watcher->inotify_fd = -1;
  watcher->stop_pipe[STOP_READ] = -1;
  watcher->stop_pipe[STOP_WRITE] = -1;

  watcher->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (watcher->inotify_fd < 0 || pipe(watcher->stop_pipe) < 0 ||
      pthread_mutex_init(&watcher->mutex, NULL) != 0) {
    if (watcher->inotify_fd >= 0) {
      close(watcher->inotify_fd);
    }
    if (watcher->stop_pipe[STOP_READ] >= 0) {
      close(watcher->stop_pipe[STOP_READ]);
      close(watcher->stop_pipe[STOP_WRITE]);
    }
    free(watcher);
    return NULL;
  }

  atomic_init(&watcher->stopped, false);

  if (pthread_create(&watcher->thread, NULL, watch_thread, watcher) != 0) {
    pthread_mutex_destroy(&watcher->mutex);
    close(watcher->inotify_fd);
    close(watcher->stop_pipe[STOP_READ]);
    close(watcher->stop_pipe[STOP_WRITE]);
    free(watcher);
    return NULL;
  }

  return watcher;
}

int file_watch_register(FileWatcher *watcher, FILE *file,
                        FileChangeCallback callback, void *userdata) {
  if (!watcher || !file || !callback || atomic_load(&watcher->stopped)) {
    return -1;
  }

  char *directory = NULL;
  char *name = NULL;
  if (!file_path(file, &directory, &name)) {
    return -1;
  }

  int watch_descriptor = inotify_add_watch(
      watcher->inotify_fd, directory,
      IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_ATTRIB);
  if (watch_descriptor < 0) {
    free(directory);
    free(name);
    return -1;
  }

  struct FileWatchEntry *entry = calloc(1, sizeof(*entry));
  if (!entry) {
    inotify_rm_watch(watcher->inotify_fd, watch_descriptor);
    free(directory);
    free(name);
    return -1;
  }

  *entry = (struct FileWatchEntry){
      .file = file,
      .callback = callback,
      .userdata = userdata,
      .watch_descriptor = watch_descriptor,
      .directory = directory,
      .name = name,
  };
  clock_gettime(CLOCK_MONOTONIC, &entry->last_callback);
  entry->has_callback = true;

  pthread_mutex_lock(&watcher->mutex);
  entry->next = watcher->entries;
  watcher->entries = entry;
  pthread_mutex_unlock(&watcher->mutex);
  wake_thread(watcher);
  invoke_callback(file, callback, userdata);
  return 0;
}

void file_watch_unregister(FileWatcher *watcher, FILE *file) {
  if (!watcher || !file) {
    return;
  }

  pthread_mutex_lock(&watcher->mutex);
  struct FileWatchEntry **cursor = &watcher->entries;

  while (*cursor) {
    struct FileWatchEntry *entry = *cursor;
    if (entry->file != file) {
      cursor = &entry->next;
      continue;
    }

    *cursor = entry->next;
    inotify_rm_watch(watcher->inotify_fd, entry->watch_descriptor);
    free(entry->directory);
    free(entry->name);
    free(entry);
    break;
  }

  pthread_mutex_unlock(&watcher->mutex);
  wake_thread(watcher);
}

void file_watch_stop(FileWatcher *watcher) {
  if (!watcher) {
    return;
  }

  atomic_store(&watcher->stopped, true);
  wake_thread(watcher);
  pthread_join(watcher->thread, NULL);

  pthread_mutex_lock(&watcher->mutex);
  struct FileWatchEntry *entry = watcher->entries;
  while (entry) {
    struct FileWatchEntry *next = entry->next;
    inotify_rm_watch(watcher->inotify_fd, entry->watch_descriptor);
    free(entry->directory);
    free(entry->name);
    free(entry);
    entry = next;
  }
  watcher->entries = NULL;
  pthread_mutex_unlock(&watcher->mutex);

  pthread_mutex_destroy(&watcher->mutex);
  close(watcher->inotify_fd);
  close(watcher->stop_pipe[STOP_READ]);
  close(watcher->stop_pipe[STOP_WRITE]);
  free(watcher);
}
