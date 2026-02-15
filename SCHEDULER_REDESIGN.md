# Scheduler Redesign Notes

## Current Architecture

### Overview
- **Atomic global sample counter** (`global_sample_position`) incremented by audio thread each block
- **Min-heap event queue** (`EventHeap`) ordered by absolute sample tick, processed by scheduler thread polling every 5ms
- **Lock-free message queue** (`msg_queue`, circular buffer of 256 entries) for passing events from scheduler to audio thread
- **Per-block message processing** in audio callback with pre/post split for sample-accurate parameter changes
- **Overflow queue** for messages that arrive too early (deferred and moved back later)

### Key Files
- `engine/scheduling.c` / `.h` — EventHeap, scheduler thread, push_event, schedule_event, defer_quant
- `engine/block_queue.c` / `.h` — msg_queue, scheduler_msg, process_msg_queue_pre/post
- `engine/ctx.c` / `.h` — Ctx struct, push_msg, pop_msg, move_overflow
- `engine/audio_loop.c` — write_callback (audio thread), get_frame_offset
- `engine/lib.c` — play_node_offset, set_input_scalar, etc. (all call push_msg directly)

### How Messages Flow (Current)
1. User code / scheduler callbacks call `push_msg(&ctx.msg_queue, msg, BUF_SIZE)` directly
2. Audio thread reads from `ctx.msg_queue` in `process_msg_queue_pre`
3. If message tick is too far in future → deferred to `ctx.overflow_queue`
4. `move_overflow()` (on scheduler thread) moves overflow msgs back to `ctx.msg_queue`

### Problems Identified

#### 1. Race Condition in overflow_queue
- `ctx.overflow_queue` is written by audio thread (`process_msg_queue_pre` line 175)
- `ctx.overflow_queue` is read by scheduler thread (`move_overflow`)
- `num_msgs` is modified non-atomically by both threads
- Works on x86 by luck (strong memory ordering), would break on ARM

#### 2. usleep(5000) Polling
- Up to 5ms latency for newly scheduled events
- Burns CPU when idle
- Can't wake early when new event is pushed

## Proposed Redesign

### Lazy Message Pushing (Eliminate Overflow Queue)

Instead of pushing all messages directly to `ctx.msg_queue` (even future ones), route them through the scheduler heap. Only push to the audio queue when the tick is imminent.

```c
// scheduling.c — new API

typedef struct {
    scheduler_msg msg;
} MsgForwardPayload;

static void forward_msg_to_audio(void *userdata, uint64_t now) {
    MsgForwardPayload *payload = (MsgForwardPayload *)userdata;
    push_msg(&ctx.msg_queue, payload->msg, 0);
    free(payload);
}

void schedule_msg(scheduler_msg msg) {
    uint64_t now = get_current_sample();
    uint64_t imminent_threshold = now + BUF_SIZE * 2;

    if (msg.tick <= imminent_threshold) {
        push_msg(&ctx.msg_queue, msg, 0);
    } else {
        MsgForwardPayload *payload = malloc(sizeof(MsgForwardPayload));
        payload->msg = msg;
        uint64_t fire_at = msg.tick - BUF_SIZE;
        push_event(forward_msg_to_audio, payload, fire_at - now, now);
    }
}
```

Then replace every `push_msg(&ctx.msg_queue, ...)` in `lib.c` with `schedule_msg(...)`.

**What this eliminates:**
- `overflow_queue` deleted entirely
- `move_overflow()` deleted
- Audio thread's `process_msg_queue_pre` drops the "too early" branch
- Clean SPSC: scheduler thread = sole writer to msg_queue, audio thread = sole reader

**Thread ownership after change:**
```
Scheduler thread (sole owner):
  ├── EventHeap (reads + writes, mutex-protected)
  ├── push_msg(&ctx.msg_queue, ...) ← sole writer
  └── process_scheduler_events → fires callbacks → may call schedule_msg

Audio thread (sole owner):
  ├── ctx.msg_queue ← sole reader (read_ptr)
  └── process_msg_queue_pre/post
```

**Note:** `set_input_scalar` and similar functions are called from user code on various threads. `schedule_msg` is safe from any thread since `push_msg` (imminent path) and `push_event` (future path, mutex-protected) are both thread-safe.

**Note:** The `BUF_SIZE` offset currently passed to `push_msg` as `buffer_offset` needs to be folded into the tick before calling `schedule_msg`, or dropped if it was just a fudge factor.

### Event-Driven Scheduler Thread (Replace usleep)

Three options explored:

#### Option 1: pthread_cond_timedwait (simplest, portable)
- Use existing mutex + condition variable
- `pthread_cond_timedwait` with deadline computed from next event's tick
- Signal with `pthread_cond_signal` when new event pushed
- Con: `CLOCK_REALTIME` can jump (NTP). Can use `pthread_condattr_setclock(CLOCK_MONOTONIC)` on Linux

#### Option 2: timerfd + eventfd + epoll (RECOMMENDED for Linux)
- `timerfd_create(CLOCK_MONOTONIC)` — kernel-managed timer, sub-μs precision
- `eventfd` — for immediate wake when new event pushed from any thread
- `epoll_wait` — blocks on both fds, zero CPU when idle
- Pro: extensible (can add OSC socket, MIDI fd to same epoll set later)
- Pro: `CLOCK_MONOTONIC` immune to system time jumps
- Con: Linux-only (macOS needs kqueue + EVFILT_TIMER, roughly 1:1 mapping)

```c
static int timer_fd, wake_fd, epoll_fd;

void scheduler_init_fds() {
    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    wake_fd  = eventfd(0, EFD_NONBLOCK);
    epoll_fd = epoll_create1(0);
    // add both fds to epoll
}

static void arm_timer(uint64_t target_tick) {
    uint64_t now = get_current_sample();
    struct itimerspec its = {0};
    if (target_tick <= now) {
        its.it_value.tv_nsec = 1;
    } else {
        double seconds = (double)(target_tick - now) / ctx_sample_rate();
        its.it_value.tv_sec  = (time_t)seconds;
        its.it_value.tv_nsec = (long)((seconds - its.it_value.tv_sec) * 1e9);
        if (its.it_value.tv_nsec == 0 && its.it_value.tv_sec == 0)
            its.it_value.tv_nsec = 1;
    }
    timerfd_settime(timer_fd, 0, &its, NULL);
}

void scheduler_wake() {
    uint64_t val = 1;
    write(wake_fd, &val, sizeof(val));
}

void *scheduler_thread_fn(void *arg) {
    struct epoll_event events[2];
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, 2, -1);
        for (int i = 0; i < nfds; i++) {
            uint64_t buf;
            read(events[i].data.fd, &buf, sizeof(buf));
        }
        pthread_mutex_lock(&scheduler_mutex);
        process_scheduler_events(get_current_sample());
        if (scheduler_queue.size > 0) {
            uint64_t next = scheduler_queue.events[0].tick;
            uint64_t fire_at = next > BUF_SIZE ? next - BUF_SIZE : 0;
            arm_timer(fire_at);
        }
        pthread_mutex_unlock(&scheduler_mutex);
    }
}
```

`push_event` calls `scheduler_wake()` + `arm_timer()` if new event is earliest.

#### Option 3: clock_nanosleep (what SuperCollider does)
- `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL)`
- Wake via `pthread_kill(thread, SIGUSR1)` to interrupt sleep (returns EINTR)
- Simple but signal handling is fiddly

### Comparison Table

| | cond_timedwait | timerfd+epoll | clock_nanosleep |
|---|---|---|---|
| Precision | ~1ms | sub-μs | sub-μs |
| Wake on new event | cond_signal | eventfd write | pthread_kill SIGUSR1 |
| Clock | REALTIME* | MONOTONIC | MONOTONIC |
| Portable | POSIX | Linux only | Linux + some POSIX |
| Extensible | No | Yes (add MIDI/OSC fds) | No |
| Complexity | Low | Medium | Low-medium |

## SuperCollider Comparison

### SC's Time-Sorted Priority Queue
- Binary min-heap, same as yalce's EventHeap
- Uses NTP timestamps (64-bit) instead of sample counts — decouples from sample rate
- Server converts NTP time → sample position when dispatching to audio thread

### SC's Three-Queue Architecture
```
[OSC input]           [Scheduler Queue]        [Audio Thread FIFO]
     |                      |                         |
Network/IPC  --->    NTP-sorted heap    --->   lock-free FIFO
(any thread)         (NRT thread)              (audio thread reads)
```

- NRT thread holds messages in heap until due, then forwards to audio FIFO
- Audio FIFO only ever contains messages for current/next block
- No overflow queue needed — timing decision happens BEFORE reaching audio thread

### Key Difference from yalce
yalce pushes to msg_queue eagerly (audio thread defers to overflow).
SC pushes lazily (scheduler holds back until right moment).

### yalce Advantages Over SC
- No IPC/serialization overhead (in-process vs OSC over UDP/TCP)
- Simpler architecture

### yalce Advantages Over PureData
- Pd is NOT sample-accurate for control messages (block-boundary only, default 64 samples)
- yalce's pre/post split gives sample-accurate parameter changes at any block size
