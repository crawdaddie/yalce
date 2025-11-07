#include "ylc_datatypes.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/event.h>
#define USE_KQUEUE 1
#else
#include <sys/epoll.h>
#define USE_EPOLL 1
#endif

// Match YALCE's type definitions
YLC_STRING_TYPE(String);

void *aalloc(size_t num_bytes) { return malloc(num_bytes); }
typedef void *(*PromiseCallback)(void *);
typedef struct {
  PromiseCallback cb;
  int32_t fd;
  int8_t op_type;
} GenericOp;

typedef void *(*CorFn)(void *);

typedef struct {
  int32_t counter;
  CorFn fn;
} Cor;

enum PType {
  ASYNC_ACCEPT = 0,
  ASYNC_READ = 1,
  ASYNC_WRITE = 2,
};
// Pending I/O operation
typedef struct PendingOp {
  void *op;
  Cor *cor;
  struct PendingOp *next;
} PendingOp;

typedef struct {
  PromiseCallback cb;
  int32_t fd;
  int8_t op_type;
  int32_t client_fd;
} AcceptPromise;

void *acc_promise_cb(void *_p) {
  AcceptPromise *p = _p;

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(p->fd, (struct sockaddr *)&client_addr, &addr_len);

  if (client_fd < 0) {
    perror("accept");
    return NULL;
  }
  p->client_fd = client_fd;
  return p;
}
void *accept_promise(int server_fd) {
  AcceptPromise *p = aalloc(sizeof(AcceptPromise));
  *p = (AcceptPromise){
      .cb = acc_promise_cb, .fd = server_fd, .op_type = ASYNC_ACCEPT};
  return p;
}

// Read Promise
typedef struct {
  PromiseCallback cb;
  int32_t fd;
  int8_t op_type;
  void *buffer;
  size_t buffer_size;
  ssize_t bytes_read;
} ReadPromise;

void *read_promise_cb(void *_p) {
  ReadPromise *p = _p;

  ssize_t n = recv(p->fd, p->buffer, p->buffer_size, 0);

  if (n < 0) {
    perror("recv");
    printf("ev failed\n");
    p->bytes_read = -1;
    return NULL;
  }

  p->bytes_read = n;

  if (n == 0) {
    // Connection closed
    printf("[ReadPromise] Connection closed on fd=%d\n", p->fd);
  } else {
    printf("[ReadPromise] Read %zd bytes from fd=%d\n", n, p->fd);
  }

  return p;
}

void *read_promise(AcceptPromise *acc, void *data_ptr, size_t size) {
  ReadPromise *p = aalloc(sizeof(ReadPromise));
  *p = (ReadPromise){.cb = read_promise_cb,
                     .op_type = ASYNC_READ,
                     .fd = acc->client_fd,
                     .buffer = data_ptr,
                     .buffer_size = size,
                     .bytes_read = 0};
  return p;
}

// Write Promise
typedef struct {
  PromiseCallback cb;
  int32_t fd;
  int8_t op_type;
  const void *data;
  size_t data_size;
  ssize_t bytes_written;
} WritePromise;

void *write_promise_cb(void *_p) {
  WritePromise *p = _p;

  printf("writing '%s'\n", (char *)p->data);
  ssize_t n = send(p->fd, p->data, p->data_size, 0);

  if (n < 0) {
    perror("send");
    p->bytes_written = -1;
    return NULL;
  }

  p->bytes_written = n;

  if (n < (ssize_t)p->data_size) {
    printf("[WritePromise] Partial write: %zd/%zu bytes on fd=%d\n", n,
           p->data_size, p->fd);
  }
  // close(p->fd);

  return p;
}

void *write_promise(ReadPromise *rp, const void *bytes_to_write, size_t size) {

  WritePromise *p = aalloc(sizeof(WritePromise));
  void *data = aalloc(sizeof(char) * size);
  memcpy(data, bytes_to_write, size);
  *p = (WritePromise){.cb = write_promise_cb,
                      .op_type = ASYNC_WRITE,
                      .fd = rp->fd,
                      .data = data,
                      .data_size = size,
                      .bytes_written = 0};
  return p;
}

// Generic promise executor (works with any promise type)
void *execute_promise(void *promise) {
  // All promises have callback at offset 0
  PromiseCallback *cb_ptr = (PromiseCallback *)promise;
  PromiseCallback cb = *cb_ptr;

  if (cb) {
    return cb(promise);
  }
  return NULL;
}

// Event loop state
typedef struct {
#ifdef USE_KQUEUE
  int kq;
#else
  int epoll_fd;
#endif
  PendingOp *pending_ops;
} EventLoop;

// Create event loop
EventLoop *event_loop_create() {
  EventLoop *loop = malloc(sizeof(EventLoop));

#ifdef USE_KQUEUE
  loop->kq = kqueue();
  if (loop->kq == -1) {
    perror("kqueue");
    free(loop);
    return NULL;
  }
  printf("[EventLoop] Created with kqueue\n");
#else
  loop->epoll_fd = epoll_create1(0);
  if (loop->epoll_fd == -1) {
    perror("epoll_create1");
    free(loop);
    return NULL;
  }
  printf("[EventLoop] Created with epoll\n");
#endif

  loop->pending_ops = NULL;
  return loop;
}

// Set socket to non-blocking
void set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Add pending operation
void add_pending_op(EventLoop *loop, GenericOp *op, Cor *cor) {
  PendingOp *pending = malloc(sizeof(PendingOp));
  pending->op = op;
  pending->cor = cor;
  pending->next = loop->pending_ops;
  loop->pending_ops = pending;

#ifdef USE_KQUEUE
  struct kevent kev;
  // ACCEPT=0 and READ=1 wait for read readiness, WRITE=2 waits for write
  // readiness
  int filter = (op->op_type == ASYNC_ACCEPT || op->op_type == ASYNC_READ)
                   ? EVFILT_READ
                   : EVFILT_WRITE;
  EV_SET(&kev, op->fd, filter, EV_ADD | EV_ONESHOT, 0, 0, pending);
  if (kevent(loop->kq, &kev, 1, NULL, 0, NULL) == -1) {
    perror("kevent add");
  }
#else
  struct epoll_event ev;
  ev.events = (op_type == 0 || op_type == 2) ? EPOLLIN : EPOLLOUT;
  ev.events |= EPOLLONESHOT;
  ev.data.ptr = pending;
  if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    perror("epoll_ctl add");
  }
#endif

  printf("[EventLoop] Registered %s on fd %d\n",
         op->op_type == ASYNC_ACCEPT ? "ACCEPT"
         : op->op_type == ASYNC_READ ? "READ"
                                     : "WRITE");
}

// Remove pending operation
PendingOp *remove_pending_op(EventLoop *loop, PendingOp *target) {
  PendingOp **curr = &loop->pending_ops;
  while (*curr) {
    if (*curr == target) {
      *curr = target->next;
      return target;
    }
    curr = &(*curr)->next;
  }
  return NULL;
}

// Register I/O operation
void event_loop_register_op(EventLoop *loop, GenericOp *op, Cor *cor) {
  printf("event loop register op %p\n", op);
  if (!op) {
    printf("[EventLoop] NULL operation\n");
    return;
  }

  printf("[EventLoop] Registering op, tag=%d\n", op->op_type);
  set_nonblocking(op->fd);
  add_pending_op(loop, op, cor);
}

// Poll for I/O events
int event_loop_poll_timeout(EventLoop *loop, int timeout_ms) {
  if (!loop->pending_ops) {
    return 0;
  }

  int completed = 0;

#ifdef USE_KQUEUE
  struct kevent events[10];
  struct timespec timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_nsec = (timeout_ms % 1000) * 1000000;

  int nev =
      kevent(loop->kq, NULL, 0, events, 10, timeout_ms == 0 ? NULL : &timeout);
  if (nev < 0) {
    perror("kevent");
    return 0;
  }

  for (int i = 0; i < nev; i++) {
    PendingOp *pending = (PendingOp *)events[i].udata;
    GenericOp *p = pending->op;

    // Check for errors
    if (events[i].flags & EV_ERROR) {
      printf("[EventLoop] Error on fd %d: %s\n", p->fd,
             strerror(events[i].data));
      remove_pending_op(loop, pending);
      free(pending);
      continue;
    }

    if (events[i].filter == EVFILT_READ || events[i].filter == EVFILT_WRITE) {
      printf("[EventLoop] Executing callback for fd=%d, op_type=%d\n", p->fd,
             p->op_type);
      GenericOp *res = execute_promise(p);
      pending->cor->fn(pending->cor);
      completed++;
    }

    remove_pending_op(loop, pending);
    free(pending);
  }
#else
  struct epoll_event events[10];
  int nfds = epoll_wait(loop->epoll_fd, events, 10, timeout_ms);
  if (nfds < 0) {
    perror("epoll_wait");
    return 0;
  }

  for (int i = 0; i < nfds; i++) {
    PendingOp *pending = (PendingOp *)events[i].data.ptr;

    if (events[i].events & EPOLLIN) {
      if (pending->op_type == 0) { // Read
        char buf[4096];
        int n = read(pending->waiting_fd, buf, sizeof(buf) - 1);

        if (n > 0) {
          buf[n] = '\0';
          char *result = malloc(n + 1);
          memcpy(result, buf, n + 1);

          pending->op->payload.read.data.size = n;
          pending->op->payload.read.data.chars = result;

          printf("[EventLoop] Read %d bytes from fd %d\n", n,
                 pending->waiting_fd);
        } else if (n == 0) {
          printf("[EventLoop] Connection closed fd %d\n", pending->waiting_fd);
          pending->op->payload.read.data.size = 0;
          pending->op->payload.read.data.chars = malloc(1);
          pending->op->payload.read.data.chars[0] = '\0';
        } else {
          perror("read");
        }

        completed++;
      } else if (pending->op_type == 2) { // Accept
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(pending->waiting_fd,
                               (struct sockaddr *)&client_addr, &addr_len);

        if (client_fd >= 0) {
          pending->op->payload.accept.client_fd = client_fd;
          printf("[EventLoop] Accepted: fd %d\n", client_fd);
          completed++;
        } else {
          perror("accept");
        }
      }
    }

    epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, pending->waiting_fd, NULL);
    remove_pending_op(loop, pending);
    free(pending);
  }
#endif

  return completed;
}

int event_loop_poll(EventLoop *loop) {
  return event_loop_poll_timeout(loop, 0);
}

// Check if has pending ops
int event_loop_has_pending(EventLoop *loop) {
  return loop->pending_ops != NULL ? 1 : 0;
}
