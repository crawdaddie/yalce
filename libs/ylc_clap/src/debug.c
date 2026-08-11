#include "./debug.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
_Thread_local ylc_plugin_t *ylc_debug_printf_context = NULL;

static void ylc_debug_append_line(ylc_plugin_t *self, const char *line) {
  if (!self || !line) {
    return;
  }

  uint32_t slot = 0;
  if (self->debug_count < YLC_DEBUG_LINES) {
    slot = self->debug_count++;
  } else {
    slot = self->debug_start;
    self->debug_start = (self->debug_start + 1) % YLC_DEBUG_LINES;
  }

  snprintf(self->debug_lines[slot], sizeof(self->debug_lines[slot]), "%u: %s",
           ++self->debug_seq, line);

  ylc_gui_draw(self);
}

static void ylc_debug_flush_partial(ylc_plugin_t *self) {
  if (!self || self->debug_partial_len == 0) {
    return;
  }

  self->debug_partial[self->debug_partial_len] = '\0';
  ylc_debug_append_line(self, self->debug_partial);
  self->debug_partial_len = 0;
}

static void ylc_debug_consume_bytes(ylc_plugin_t *self, const char *bytes,
                                    ssize_t count) {
  if (!self || !bytes || count <= 0) {
    return;
  }

  for (ssize_t i = 0; i < count; ++i) {
    const char ch = bytes[i];
    if (ch == '\n') {
      ylc_debug_flush_partial(self);
      continue;
    }

    if (self->debug_partial_len + 1 >= sizeof(self->debug_partial)) {
      ylc_debug_flush_partial(self);
    }

    if (ch >= 32 || ch == '\t') {
      self->debug_partial[self->debug_partial_len++] = ch;
    }
  }
}

void ylc_debug_drain_pipe(ylc_plugin_t *self) {
  if (!self || self->debug_pipe_read_fd < 0) {
    return;
  }

  char buffer[1024];
  for (;;) {
    const ssize_t bytes =
        read(self->debug_pipe_read_fd, buffer, sizeof(buffer));
    if (bytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return;
      }
      ylc_debug_append_line(self, "debug pipe read failed");
      return;
    }
    if (bytes == 0) {
      return;
    }

    ylc_debug_consume_bytes(self, buffer, bytes);
  }
}

void ylc_debug_log(ylc_plugin_t *self, const char *format, ...) {
  if (!self || !format) {
    return;
  }

  char message[1024] = {0};
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  if (self->debug_stream) {
    fputs(message, self->debug_stream);
    fputc('\n', self->debug_stream);
    fflush(self->debug_stream);
  } else {
    ylc_debug_append_line(self, message);
  }

  if (self->debug_log_file) {
    fputs(message, self->debug_log_file);
    fputc('\n', self->debug_log_file);
    fflush(self->debug_log_file);
  }

  if (!self->destroying && self->clap_initialized && self->host &&
      self->host->request_callback) {
    self->host->request_callback(self->host);
  }
}

static void ylc_debug_log_text(ylc_plugin_t *self, const char *text) {
  if (!self || !text) {
    return;
  }

  const char *line = text;
  while (*line) {
    const char *end = strchr(line, '\n');
    const char *start = line;
    size_t len = end ? (size_t)(end - line) : strlen(line);
    while (len > 0) {
      const size_t chunk_len = len > 900 ? 900 : len;
      ylc_debug_log(self, "%.*s", (int)chunk_len, line);
      line += chunk_len;
      len -= chunk_len;
    }
    if (end) {
      if (end == start) {
        ylc_debug_log(self, "");
      }
      line = end + 1;
    } else {
      break;
    }
  }
}

static void ylc_debug_flush_script_text(ylc_plugin_t *self) {
  if (!self || self->debug_partial_len == 0) {
    return;
  }

  self->debug_partial[self->debug_partial_len] = '\0';
  ylc_debug_log(self, "%s", self->debug_partial);
  self->debug_partial_len = 0;
}

static void ylc_debug_log_script_text(ylc_plugin_t *self, const char *text) {
  if (!self || !text) {
    return;
  }

  for (const char *cursor = text; *cursor; ++cursor) {
    const char ch = *cursor;
    if (ch == '\n') {
      if (self->debug_partial_len > 0) {
        ylc_debug_flush_script_text(self);
      } else {
        ylc_debug_log(self, "");
      }
      continue;
    }

    if (self->debug_partial_len + 1 >= sizeof(self->debug_partial)) {
      ylc_debug_flush_script_text(self);
    }

    if (ch >= 32 || ch == '\t') {
      self->debug_partial[self->debug_partial_len++] = ch;
    }
  }
}

void ylc_debug_compile_log(void *user_data, const char *line) {
  ylc_debug_log_text((ylc_plugin_t *)user_data, line);
}

void ylc_plugin_debug_printf_set_context(void *plugin_state) {
  ylc_debug_printf_context = (ylc_plugin_t *)plugin_state;
}

void ylc_plugin_debug_printf_clear_context(void *plugin_state) {
  if (!plugin_state ||
      ylc_debug_printf_context == (ylc_plugin_t *)plugin_state) {
    ylc_debug_printf_context = NULL;
  }
}

int ylc_plugin_debug_printf(const char *format, ...) {
  if (!format) {
    return 0;
  }

  char message[4096] = {0};
  va_list args;
  va_start(args, format);
  const int written = vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  if (ylc_debug_printf_context) {
    ylc_debug_log_script_text(ylc_debug_printf_context, message);
  } else {
    fputs(message, stderr);
    fflush(stderr);
  }

  return written;
}

int ylc_plugin_debug_fprintf(FILE *stream, const char *format, ...) {
  if (!format) {
    return 0;
  }

  char message[4096] = {0};
  va_list args;
  va_start(args, format);
  const int written = vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  if (ylc_debug_printf_context) {
    ylc_debug_log_script_text(ylc_debug_printf_context, message);
  } else if (stream) {
    fputs(message, stream);
    fflush(stream);
  }

  return written;
}

int ylc_plugin_debug_fflush(FILE *stream) {
  if (ylc_debug_printf_context) {
    ylc_debug_flush_script_text(ylc_debug_printf_context);
    return 0;
  }

  return fflush(stream);
}

bool ylc_open_debug_log_file(ylc_plugin_t *self, const char *path,
                             int *open_errno) {
  if (!self || !path || path[0] == '\0') {
    return false;
  }

  snprintf(self->debug_log_path, sizeof(self->debug_log_path), "%s", path);

  FILE *file = fopen(path, "a");
  if (!file) {
    if (open_errno) {
      *open_errno = errno;
    }
    return false;
  }

  setvbuf(file, NULL, _IOLBF, 0);
  self->debug_log_file = file;
  return true;
}

void ylc_close_debug_log_file(ylc_plugin_t *self) {
  if (!self || !self->debug_log_file) {
    return;
  }

  fclose(self->debug_log_file);
  self->debug_log_file = NULL;
}

void ylc_close_debug_pipe(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  const int read_fd = self->debug_pipe_read_fd;
  const int write_fd = self->debug_pipe_write_fd;
  const bool registered = self->debug_pipe_registered;
  FILE *stream = self->debug_stream;

  self->debug_pipe_read_fd = -1;
  self->debug_pipe_write_fd = -1;
  self->debug_pipe_registered = false;
  self->debug_stream = NULL;

  if (registered && self->host_posix_fd && self->host_posix_fd->unregister_fd &&
      read_fd >= 0) {
    self->host_posix_fd->unregister_fd(self->host, read_fd);
  }

  if (stream) {
    fclose(stream);
  } else if (write_fd >= 0) {
    close(write_fd);
  }

  if (read_fd >= 0) {
    close(read_fd);
  }
}

void ylc_open_debug_pipe(ylc_plugin_t *self) {
  if (!self) {
    return;
  }

  int fds[2] = {-1, -1};
  if (pipe(fds) != 0) {
    ylc_debug_append_line(self, "debug pipe creation failed");
    return;
  }

  fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL, 0) | O_NONBLOCK);
  fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK);
  fcntl(fds[0], F_SETFD, fcntl(fds[0], F_GETFD, 0) | FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, fcntl(fds[1], F_GETFD, 0) | FD_CLOEXEC);

  self->debug_pipe_read_fd = fds[0];
  self->debug_pipe_write_fd = fds[1];
  self->debug_stream = fdopen(self->debug_pipe_write_fd, "w");
  if (!self->debug_stream) {
    ylc_close_debug_pipe(self);
    ylc_debug_append_line(self, "debug stream creation failed");
    return;
  }

  setvbuf(self->debug_stream, NULL, _IOLBF, 0);
}

void ylc_register_debug_pipe(ylc_plugin_t *self) {
  if (!self || self->debug_pipe_read_fd < 0 || !self->host_posix_fd ||
      !self->host_posix_fd->register_fd) {
    return;
  }

  self->debug_pipe_registered = self->host_posix_fd->register_fd(
      self->host, self->debug_pipe_read_fd, CLAP_POSIX_FD_READ);
}
