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

YLC_STRING_TYPE(String);

int set_socket_reuse(int sockfd) {
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt SO_REUSEADDR");
    return -1;
  }
  return 0;
}

enum PType {
  ASYNC_ACCEPT = 0,
  ASYNC_READ = 1,
  ASYNC_WRITE = 2,
};

void *io_alloc(size_t num_bytes) { return malloc(num_bytes); }
typedef void *(*PromiseCallback)(void *);

typedef void *(*CorFn)(void *);

typedef struct {
  int8_t tag;
  void *val;
} CorPromise;

typedef struct {
  int32_t counter;
  CorFn fn;
  void *state;
  struct Cor *next;
  CorPromise promise;
} Cor;

// Pending I/O operation
typedef struct PendingOp {
  Cor *cor;
  int32_t waiting_fd;
  int8_t op_type;
  void *result_ptr;
  PromiseCallback cb;
  struct PendingOp *next;

} PendingOp;

typedef struct {
#ifdef USE_KQUEUE
  int kq;
#else
  int epoll_fd;
#endif

  PendingOp *pending_ops;
} EventLoop;

void event_loop_register_coroutine(EventLoop *loop, Cor *cor);
void event_loop_register_op(EventLoop *loop, PendingOp *op);

static EventLoop *_loop;
typedef struct {
  int32_t s;
  struct {
    int32_t s;
    int *p;
  } ref;
} initial_state;

Cor *clone_coroutine(Cor *cor) {
  Cor *new = io_alloc(sizeof(Cor));

  // TODO: copy state as well (just don't know really how big it is)
  *new = *cor;
  new->state = io_alloc(512 * sizeof(char));
  memcpy(new->state, cor->state, 512);

  new->counter = 0;
  new->promise.tag = 1;
  new->promise.val = NULL;
  return new;
}

void *io_accept_callback(PendingOp *op) {

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  int client_fd =
      accept(op->waiting_fd, (struct sockaddr *)&client_addr, &addr_len);

  if (client_fd < 0) {
    perror("accept");
    return NULL;
  }
  // printf("accept on %d got %d\n", op->waiting_fd, client_fd);
  *((int32_t *)op->result_ptr) = client_fd;
  Cor *cloned = clone_coroutine(op->cor);
  event_loop_register_coroutine(_loop, cloned);
  return op;
}

void *io_accept(int server_fd, int *client_fd_ref, void *cor) {
  if (!_loop) {
    fprintf(stderr, "event loop error: no current event loop exists\n");
    return NULL;
  }

  PendingOp *op = io_alloc(sizeof(PendingOp));
  op->waiting_fd = server_fd;
  op->result_ptr = client_fd_ref;
  op->cor = cor;
  op->op_type = ASYNC_ACCEPT;
  op->cb = (PromiseCallback)io_accept_callback;

  event_loop_register_op(_loop, op);

  return op;
}
void *io_read_cb(PendingOp *op) {

  ssize_t n = recv(op->waiting_fd, op->result_ptr, 4096, 0);

  if (n < 0) {
    perror("recv");
    printf("ev failed\n");
    return NULL;
  }

  if (n == 0) {
    // Connection closed
    // printf("[ReadPromise] Connection closed on fd=%d\n", op->waiting_fd);
  } else {
    // printf("[ReadPromise] Read %zd bytes from fd=%d\n", n, op->waiting_fd);
  }
  return op;
}
void print_addr(void *p) { printf("addr: %p\n", p); }

void *io_read(int client_fd, char *read_res, void *cor) {

  if (!_loop) {
    fprintf(stderr, "event loop error: no current event loop exists\n");
    return NULL;
  }

  PendingOp *op = io_alloc(sizeof(PendingOp));
  op->waiting_fd = client_fd;
  op->result_ptr = read_res;
  op->cor = cor;
  op->op_type = ASYNC_READ;
  op->cb = (PromiseCallback)io_read_cb;

  event_loop_register_op(_loop, op);
  return op;
}

typedef struct PendingOpWrite {
  Cor *cor;
  int32_t waiting_fd;
  int8_t op_type;
  void *result_ptr;
  PromiseCallback cb;
  struct PendingOp *next;
  int size;
  char *to_write;
} PendingOpWrite;

void *io_write_cb(PendingOpWrite *op) {

  ssize_t n = send(op->waiting_fd, op->to_write, op->size, 0);

  if (n < 0) {
    perror("send");
    return NULL;
  }

  if (n < (ssize_t)op->size) {
  }
  free(op->to_write);
  return op;
}

void *io_write(int client_fd, char *to_write, void *cor) {

  if (!_loop) {
    fprintf(stderr, "event loop error: no current event loop exists\n");
    return NULL;
  }
  int len = strlen(to_write);

  PendingOpWrite *op = io_alloc(sizeof(PendingOpWrite));
  op->waiting_fd = client_fd;
  op->cor = cor;
  op->op_type = ASYNC_WRITE;
  op->cb = (PromiseCallback)io_write_cb;
  // op->to_write = io_alloc((len + 1) * sizeof(char));
  op->to_write = to_write;
  op->size = len;

  event_loop_register_op(_loop, op);
  return op;
}

void cor_resume(Cor *cor) { cor->fn(cor); }
void event_loop_register_coroutine(EventLoop *loop, Cor *cor) {
  cor_resume(cor);
}

void *event_loop_create() {

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

  _loop = loop;
  return loop;
}

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

int event_loop_poll(EventLoop *loop) {
  if (!loop->pending_ops) {
    return 0;
  }

  int completed = 0;

#ifdef USE_KQUEUE
  struct kevent events[10];

  int nev = kevent(loop->kq, NULL, 0, events, 10, NULL);
  if (nev < 0) {
    perror("kevent");
    return 0;
  }

  for (int i = 0; i < nev; i++) {
    PendingOp *pending = (PendingOp *)events[i].udata;

    if (events[i].flags & EV_ERROR) {
      printf("[EventLoop] Error on fd %d: %s\n", pending->waiting_fd,
             strerror(events[i].data));
      remove_pending_op(loop, pending);
      free(pending);
      continue;
    }

    if (events[i].filter == EVFILT_READ || events[i].filter == EVFILT_WRITE) {
      pending->cb(pending);
      cor_resume(pending->cor);
      if (pending->cor->counter == -1) {
        close(
            pending->waiting_fd); // Close the client fd when coroutine finishes
        free(pending->cor);
      }
      completed++;
    }

    remove_pending_op(loop, pending);
    free(pending);
  }
#else
  struct epoll_event events[10];

  int nev = epoll_wait(loop->epoll_fd, events, 10, -1);
  if (nev < 0) {
    perror("epoll_wait");
    return 0;
  }

  for (int i = 0; i < nev; i++) {
    PendingOp *pending = (PendingOp *)events[i].data.ptr;

    if (events[i].events & (EPOLLERR | EPOLLHUP)) {
      printf("[EventLoop] Error on fd %d\n", pending->waiting_fd);
      remove_pending_op(loop, pending);
      free(pending);
      continue;
    }

    if (events[i].events & (EPOLLIN | EPOLLOUT)) {
      pending->cb(pending);
      cor_resume(pending->cor);
      if (pending->cor->counter == -1) {
        close(
            pending->waiting_fd); // Close the client fd when coroutine finishes
        free(pending->cor);
      }
      completed++;
    }

    remove_pending_op(loop, pending);
    free(pending);
  }
#endif

  return completed;
}

// Set socket to non-blocking
void set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void event_loop_register_op(EventLoop *loop, PendingOp *op) {
  set_nonblocking(op->waiting_fd);

  op->next = loop->pending_ops;
  loop->pending_ops = op;

#ifdef USE_KQUEUE
  struct kevent kev;
  // ACCEPT=0 and READ=1 wait for read readiness, WRITE=2 waits for write
  // readiness
  int filter = (op->op_type == ASYNC_ACCEPT || op->op_type == ASYNC_READ)
                   ? EVFILT_READ
                   : EVFILT_WRITE;
  EV_SET(&kev, op->waiting_fd, filter, EV_ADD | EV_ONESHOT, 0, 0, op);
  if (kevent(loop->kq, &kev, 1, NULL, 0, NULL) == -1) {
    perror("kevent add");
  }
#else
  struct epoll_event ev;
  ev.events = (op->op_type == ASYNC_ACCEPT || op->op_type == ASYNC_READ)
                  ? EPOLLIN
                  : EPOLLOUT;
  ev.events |= EPOLLONESHOT;
  ev.data.ptr = op;
  if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, op->waiting_fd, &ev) == -1) {
    perror("epoll_ctl add");
  }
#endif
}

void event_loop_run(EventLoop *loop) {

  printf("[EventLoop] Running ...\n");
  while (1) {
    event_loop_poll(loop);
  }
}
