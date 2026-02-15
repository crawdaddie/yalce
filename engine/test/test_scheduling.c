/*
 * Tests for scheduling (EventHeap) and audio queue (msg_queue,
 * process_msg_queue_pre/post) code.
 *
 * Build: make -C engine test
 *
 * We #include the .c files directly to access static functions.
 * External dependencies are stubbed below.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Headers (types only) ─────────────────────────────────── */

#include "../audio_graph.h"
#include "../block_queue.h"
#include "../common.h"
#include "../ctx.h"
#include "../node.h"

/* ── Globals required by headers ──────────────────────────── */

Ctx ctx;

/* audio_graph.h externs */
AudioGraph *_graph = NULL;
Node *_chain_head = NULL;
Node *_chain_tail = NULL;

/* ── Stubs for functions we don't need ────────────────────── */

void perform_audio_graph(Node *_node, AudioGraph *graph, Node *_inputs[],
                         int nframes, double spf) {
  (void)_node;
  (void)graph;
  (void)_inputs;
  (void)nframes;
  (void)spf;
}

NodeRef group_add(NodeRef node, NodeRef group) {
  (void)node;
  (void)group;
  return NULL;
}

void audio_ctx_add(Node *node) { (void)node; }
void offset_node_bufs(Node *node, int f) {
  (void)node;
  (void)f;
}
void unoffset_node_bufs(Node *node, int f) {
  (void)node;
  (void)f;
}

double pow2table_read(double pos, int tabsize, double *table) {
  (void)pos;
  (void)tabsize;
  (void)table;
  return 0.0;
}

/* ── msg_queue implementation (from ctx.c, standalone) ────── */

void push_msg(msg_queue *queue, scheduler_msg msg, int buffer_offset) {
  msg.tick += buffer_offset;
  if (queue->num_msgs == MSG_QUEUE_MAX_SIZE) {
    return; /* full — silently drop */
  }
  *(queue->buffer + queue->write_ptr) = msg;
  queue->write_ptr = (queue->write_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs++;
}

scheduler_msg pop_msg(msg_queue *queue) {
  scheduler_msg msg = *(queue->buffer + queue->read_ptr);
  queue->read_ptr = (queue->read_ptr + 1) % MSG_QUEUE_MAX_SIZE;
  queue->num_msgs--;
  return msg;
}

void move_overflow(void) {
  msg_queue *queue = &ctx.overflow_queue;
  scheduler_msg msg;
  while (queue->num_msgs) {
    msg = pop_msg(queue);
    push_msg(&(ctx.msg_queue), msg, 0);
  }
}

int ctx_sample_rate(void) { return 48000; }
double ctx_spf(void) { return 1.0 / 48000.0; }

/* ── Include source files under test ──────────────────────── */

/* scheduling.c — gives us EventHeap, push_event, pop_event,
   process_scheduler_events, schedule_event, defer_quant, etc. */
#include "../scheduling.c"

/* block_queue.c — gives us process_msg_queue_pre/post */
#include "../block_queue.c"

/* ── Test helpers ─────────────────────────────────────────── */

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn)                                                           \
  do {                                                                         \
    tests_run++;                                                               \
    printf("  %-50s ", #fn);                                                   \
    fflush(stdout);                                                            \
    fn();                                                                      \
    tests_passed++;                                                            \
    printf("OK\n");                                                            \
  } while (0)

static void reset_heap(void) {
  scheduler_queue.size = 0;
  /* keep existing allocation */
}

static void reset_msg_queue(msg_queue *q) { memset(q, 0, sizeof(msg_queue)); }

static void reset_all(void) {
  reset_heap();
  reset_msg_queue(&ctx.msg_queue);
  reset_msg_queue(&ctx.overflow_queue);
  atomic_store(&global_sample_position, 0);
}

/*
 * Create a minimal Node + AudioGraph that process_msg_pre/post can operate on.
 * Memory layout: [Node][AudioGraph][inlet_nodes...]
 * Returns the outer Node*. Caller must free().
 */
typedef struct {
  Node node;
  AudioGraph graph;
  Node inlet_nodes[MAX_INPUTS];
  double inlet_bufs[MAX_INPUTS][BUF_SIZE];
} MockSynth;

static MockSynth *create_mock_synth(int num_inlets) {
  MockSynth *m = calloc(1, sizeof(MockSynth));

  m->node.perform = (perform_func_t)perform_audio_graph;
  /* block_queue.c does: AudioGraph *g = (AudioGraph *)((Node *)node + 1)
     but then checks node->state_ptr first. We use state_ptr to point
     to our graph so the memory layout doesn't matter. */
  m->node.state_ptr = &m->graph;

  m->graph.num_inlets = num_inlets;
  m->graph.nodes = m->inlet_nodes;

  for (int i = 0; i < num_inlets; i++) {
    m->graph.inlets[i] = i; /* inlet i → inlet_nodes[i] */
    m->inlet_nodes[i].output.buf = m->inlet_bufs[i];
    m->inlet_nodes[i].output.layout = 1;
    m->inlet_nodes[i].output.size = BUF_SIZE;
  }

  return m;
}

static void clear_inlet_buf(MockSynth *m, int inlet) {
  memset(m->inlet_bufs[inlet], 0, sizeof(double) * BUF_SIZE);
}

/* ══════════════════════════════════════════════════════════════
   LAYER 1: Pure Data Structure Tests
   ══════════════════════════════════════════════════════════════ */

/* ── EventHeap tests ──────────────────────────────────────── */

static uint64_t recorded_ticks[64];
static int recorded_count;

static void record_tick_cb(void *ud, uint64_t tick) {
  (void)ud;
  recorded_ticks[recorded_count++] = tick;
}

static void test_heap_insert_pop_order(void) {
  reset_all();
  /* Insert out of order */
  push_event(record_tick_cb, (void *)1, 300, 0);
  push_event(record_tick_cb, (void *)1, 100, 0);
  push_event(record_tick_cb, (void *)1, 200, 0);

  assert(scheduler_queue.size == 3);

  SchedulerEvent e1 = pop_event(&scheduler_queue);
  SchedulerEvent e2 = pop_event(&scheduler_queue);
  SchedulerEvent e3 = pop_event(&scheduler_queue);

  assert(e1.tick == 100);
  assert(e2.tick == 200);
  assert(e3.tick == 300);
  assert(scheduler_queue.size == 0);
}

static void test_heap_pop_empty(void) {
  reset_all();
  SchedulerEvent e = pop_event(&scheduler_queue);
  assert(e.callback == NULL);
  assert(e.userdata == NULL);
}

static void test_heap_duplicate_ticks(void) {
  reset_all();
  push_event(record_tick_cb, (void *)1, 50, 0);
  push_event(record_tick_cb, (void *)2, 50, 0);
  push_event(record_tick_cb, (void *)3, 50, 0);

  assert(scheduler_queue.size == 3);

  SchedulerEvent e1 = pop_event(&scheduler_queue);
  SchedulerEvent e2 = pop_event(&scheduler_queue);
  SchedulerEvent e3 = pop_event(&scheduler_queue);

  /* All should have tick 50 */
  assert(e1.tick == 50);
  assert(e2.tick == 50);
  assert(e3.tick == 50);
  assert(scheduler_queue.size == 0);
}

static void test_heap_single_element(void) {
  reset_all();
  push_event(record_tick_cb, (void *)1, 42, 0);
  assert(scheduler_queue.size == 1);

  SchedulerEvent e = pop_event(&scheduler_queue);
  assert(e.tick == 42);
  assert(scheduler_queue.size == 0);
}

static void test_heap_grow_past_capacity(void) {
  reset_all();
  /* Initial capacity is 64, push 100 events */
  for (int i = 0; i < 100; i++) {
    push_event(record_tick_cb, (void *)(uintptr_t)i, (uint64_t)(100 - i), 0);
  }
  assert(scheduler_queue.size == 100);
  assert(scheduler_queue.capacity >= 100);

  /* Verify they come out in ascending tick order */
  uint64_t prev_tick = 0;
  for (int i = 0; i < 100; i++) {
    SchedulerEvent e = pop_event(&scheduler_queue);
    assert(e.tick >= prev_tick);
    prev_tick = e.tick;
  }
  assert(scheduler_queue.size == 0);
}

static void test_heap_large_batch(void) {
  reset_all();
  /* Insert 1000 events with random-ish ticks */
  for (int i = 0; i < 1000; i++) {
    uint64_t tick = (uint64_t)((i * 7919) % 10000); /* pseudo-random */
    push_event(record_tick_cb, (void *)1, tick, 0);
  }
  assert(scheduler_queue.size == 1000);

  uint64_t prev = 0;
  for (int i = 0; i < 1000; i++) {
    SchedulerEvent e = pop_event(&scheduler_queue);
    assert(e.tick >= prev);
    prev = e.tick;
  }
  assert(scheduler_queue.size == 0);
}

static void test_heap_interleaved_push_pop(void) {
  reset_all();
  push_event(record_tick_cb, (void *)1, 50, 0);
  push_event(record_tick_cb, (void *)1, 10, 0);

  SchedulerEvent e = pop_event(&scheduler_queue);
  assert(e.tick == 10);

  push_event(record_tick_cb, (void *)1, 30, 0);

  e = pop_event(&scheduler_queue);
  assert(e.tick == 30);

  e = pop_event(&scheduler_queue);
  assert(e.tick == 50);

  assert(scheduler_queue.size == 0);
}

/* ── msg_queue tests ──────────────────────────────────────── */

static void test_queue_push_pop_single(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 100};
  msg.payload.NODE_SET_SCALAR.value = 0.75;

  push_msg(q, msg, 0);
  assert(q->num_msgs == 1);

  scheduler_msg out = pop_msg(q);
  assert(out.tick == 100);
  assert(out.type == NODE_SET_SCALAR);
  assert(out.payload.NODE_SET_SCALAR.value == 0.75);
  assert(q->num_msgs == 0);
}

static void test_queue_fill_to_capacity(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  for (int i = 0; i < MSG_QUEUE_MAX_SIZE; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)i};
    push_msg(q, msg, 0);
  }
  assert(q->num_msgs == MSG_QUEUE_MAX_SIZE);

  /* Verify all round-trip */
  for (int i = 0; i < MSG_QUEUE_MAX_SIZE; i++) {
    scheduler_msg out = pop_msg(q);
    assert(out.tick == (uint64_t)i);
  }
  assert(q->num_msgs == 0);
}

static void test_queue_overflow_drops(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  /* Fill to capacity */
  for (int i = 0; i < MSG_QUEUE_MAX_SIZE; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)i};
    push_msg(q, msg, 0);
  }
  assert(q->num_msgs == MSG_QUEUE_MAX_SIZE);

  /* This should be silently dropped */
  scheduler_msg extra = {.type = NODE_SET_SCALAR, .tick = 9999};
  push_msg(q, extra, 0);
  assert(q->num_msgs == MSG_QUEUE_MAX_SIZE);

  /* First message should still be tick 0, not 9999 */
  scheduler_msg out = pop_msg(q);
  assert(out.tick == 0);
}

static void test_queue_wraparound(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  /* Push 200, pop 200 */
  for (int i = 0; i < 200; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)i};
    push_msg(q, msg, 0);
  }
  for (int i = 0; i < 200; i++) {
    scheduler_msg out = pop_msg(q);
    assert(out.tick == (uint64_t)i);
  }
  assert(q->num_msgs == 0);

  /* Now write_ptr and read_ptr are at 200. Push another 200. */
  for (int i = 0; i < 200; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)(1000 + i)};
    push_msg(q, msg, 0);
  }
  assert(q->num_msgs == 200);

  for (int i = 0; i < 200; i++) {
    scheduler_msg out = pop_msg(q);
    assert(out.tick == (uint64_t)(1000 + i));
  }
  assert(q->num_msgs == 0);
}

static void test_queue_buffer_offset(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 100};
  push_msg(q, msg, 50); /* should add 50 to tick */

  scheduler_msg out = pop_msg(q);
  assert(out.tick == 150);
}

static void test_queue_num_msgs_tracking(void) {
  reset_all();
  msg_queue *q = &ctx.msg_queue;

  assert(q->num_msgs == 0);

  for (int i = 0; i < 10; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 0};
    push_msg(q, msg, 0);
    assert(q->num_msgs == i + 1);
  }

  for (int i = 9; i >= 0; i--) {
    pop_msg(q);
    assert(q->num_msgs == i);
  }
}

/* ══════════════════════════════════════════════════════════════
   LAYER 1.5: Scheduler Logic (single-threaded, no real threads)
   ══════════════════════════════════════════════════════════════ */

static void test_process_events_fires_due(void) {
  reset_all();

  recorded_count = 0;
  push_event(record_tick_cb, (void *)1, 100, 0);
  push_event(record_tick_cb, (void *)1, 200, 0);
  push_event(record_tick_cb, (void *)1, 300, 0);

  /* Process at tick 150: only tick 100 should fire */
  process_scheduler_events(150);

  assert(recorded_count == 1);
  assert(recorded_ticks[0] == 100);

  /* Process at 300: ticks 200 and 300 fire */
  process_scheduler_events(300);

  assert(recorded_count == 3);
  assert(recorded_ticks[1] == 200);
  assert(recorded_ticks[2] == 300);
}

static void test_process_events_none_due(void) {
  reset_all();
  recorded_count = 0;

  push_event(record_tick_cb, (void *)1, 500, 0);

  process_scheduler_events(100);

  assert(recorded_count == 0);
  assert(scheduler_queue.size == 1);
}

/* Test that a callback can re-schedule without corrupting the heap */
static void reschedule_cb(void *ud, uint64_t tick) {
  int *count = (int *)ud;
  (*count)++;
  if (*count < 3) {
    /* Re-schedule 100 samples later */
    push_event(reschedule_cb, ud, 100, tick);
  }
}

static void test_process_events_reschedule(void) {
  reset_all();
  int count = 0;

  push_event(reschedule_cb, &count, 100, 0);

  /* Fire at tick 100 → callback pushes event at tick 200 */
  process_scheduler_events(100);
  assert(count == 1);

  /* Fire at tick 200 → callback pushes event at tick 300 */
  process_scheduler_events(200);
  assert(count == 2);

  /* Fire at tick 300 → callback does NOT re-schedule (count reaches 3) */
  process_scheduler_events(300);
  assert(count == 3);
  assert(scheduler_queue.size == 0);
}

static void test_defer_quant_alignment(void) {
  reset_all();
  ctx.sample_rate = 48000;

  /* At sample 0, quant 1.0s → target = 48000 */
  atomic_store(&global_sample_position, 0);
  defer_quant(1.0, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 48000);

  reset_heap();

  /* At sample 1000, quant 0.5s (24000 samples) → next boundary = 24000 */
  atomic_store(&global_sample_position, 1000);
  defer_quant(0.5, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 24000);

  reset_heap();

  /* At exact boundary: sample 48000, quant 1.0s → next boundary = 96000 */
  atomic_store(&global_sample_position, 48000);
  defer_quant(1.0, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 96000);
}

static void test_schedule_event_conversion(void) {
  reset_all();
  ctx.sample_rate = 48000;

  /* 1 second delay from tick 0 → tick 48000 */
  int dummy = 42;
  schedule_event(0, 1.0, record_tick_cb, &dummy);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 48000);

  reset_heap();

  /* NULL userdata → early return, nothing pushed */
  schedule_event(0, 1.0, record_tick_cb, NULL);
  assert(scheduler_queue.size == 0);
}

/* ══════════════════════════════════════════════════════════════
   LAYER 2: Message Processing (pre/post with mock nodes)
   ══════════════════════════════════════════════════════════════ */

static void test_pre_on_time_scalar(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  /* Message at tick 10, current_tick 0, frame_count = BUF_SIZE
     → frame_offset = 10 → fills buf[10..BUF_SIZE-1] with 0.5 */
  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 10};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.5;

  push_msg(&ctx.msg_queue, msg, 0);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  double *buf = m->inlet_bufs[0];
  /* Samples 0..9 should be untouched (0.0) */
  for (int i = 0; i < 10; i++) {
    assert(buf[i] == 0.0);
  }
  /* Samples 10..BUF_SIZE-1 should be 0.5 */
  for (int i = 10; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.5);
  }

  free(m);
}

static void test_post_on_time_scalar(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 10};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.5;

  push_msg(&ctx.msg_queue, msg, 0);

  /* Pre fills [10..end], post fills [0..9] */
  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  process_msg_queue_post(0, BUF_SIZE, &ctx.msg_queue, consumed);

  double *buf = m->inlet_bufs[0];
  /* After pre+post, entire buffer should be 0.5 */
  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.5);
  }

  free(m);
}

static void test_pre_late_message(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  /* Message at tick 50, but current_tick is 100 → late, offset 0 */
  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 50};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.9;

  push_msg(&ctx.msg_queue, msg, 0);

  int consumed = process_msg_queue_pre(100, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  /* Late message processed at offset 0 → entire buffer filled */
  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.9);
  }

  free(m);
}

static void test_pre_early_message_deferred(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  /* Message at tick 1000, current_tick 0, frame_count 256
     → tick - current >= frame_count → deferred to overflow */
  scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = 1000};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.3;

  push_msg(&ctx.msg_queue, msg, 0);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  /* Buffer should be untouched */
  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.0);
  }

  /* Message should have been pushed to overflow queue */
  assert(ctx.overflow_queue.num_msgs == 1);
  scheduler_msg overflow_msg = pop_msg(&ctx.overflow_queue);
  assert(overflow_msg.tick == 1000);

  free(m);
}

static void test_pre_post_trig(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  scheduler_msg msg = {.type = NODE_SET_TRIG, .tick = 20};
  msg.payload.NODE_SET_TRIG.target = &m->node;
  msg.payload.NODE_SET_TRIG.input = 0;

  push_msg(&ctx.msg_queue, msg, 0);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  double *buf = m->inlet_bufs[0];
  /* Pre: buf[20] = 1.0, everything else 0.0 */
  for (int i = 0; i < BUF_SIZE; i++) {
    if (i == 20) {
      assert(buf[i] == 1.0);
    } else {
      assert(buf[i] == 0.0);
    }
  }

  /* Post: buf[20] = 0.0 (trig cleared) */
  process_msg_queue_post(0, BUF_SIZE, &ctx.msg_queue, consumed);
  assert(buf[20] == 0.0);

  free(m);
}

static void test_pre_multiple_messages_ordering(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  /* Two scalar messages: first sets 0.5 at offset 0, second sets 0.8 at offset
     128. After pre, buf should be: [0..127]  = 0.5 (first msg fills from 0,
     then second overwrites nothing here since it starts at 128) wait no — first
     msg fills [0..end] with 0.5, then second msg fills [128..end] with 0.8 So:
     [0..127] = 0.5, [128..end] = 0.8 */
  scheduler_msg msg1 = {.type = NODE_SET_SCALAR, .tick = 0};
  msg1.payload.NODE_SET_SCALAR.target = &m->node;
  msg1.payload.NODE_SET_SCALAR.input = 0;
  msg1.payload.NODE_SET_SCALAR.value = 0.5;

  scheduler_msg msg2 = {.type = NODE_SET_SCALAR, .tick = 128};
  msg2.payload.NODE_SET_SCALAR.target = &m->node;
  msg2.payload.NODE_SET_SCALAR.input = 0;
  msg2.payload.NODE_SET_SCALAR.value = 0.8;

  push_msg(&ctx.msg_queue, msg1, 0);
  push_msg(&ctx.msg_queue, msg2, 0);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 2);

  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < 128; i++) {
    assert(buf[i] == 0.5);
  }
  for (int i = 128; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.8);
  }

  free(m);
}

static void test_pre_post_full_buffer_coverage(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);

  /* Test several different offsets to ensure pre+post always covers full buffer
   */
  int offsets[] = {0, 1, BUF_SIZE / 4, BUF_SIZE / 2, BUF_SIZE - 1};
  int noffsets = sizeof(offsets) / sizeof(offsets[0]);

  for (int t = 0; t < noffsets; t++) {
    int offset = offsets[t];
    clear_inlet_buf(m, 0);
    reset_msg_queue(&ctx.msg_queue);
    reset_msg_queue(&ctx.overflow_queue);

    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)offset};
    msg.payload.NODE_SET_SCALAR.target = &m->node;
    msg.payload.NODE_SET_SCALAR.input = 0;
    msg.payload.NODE_SET_SCALAR.value = 1.0;

    push_msg(&ctx.msg_queue, msg, 0);

    int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
    process_msg_queue_post(0, BUF_SIZE, &ctx.msg_queue, consumed);

    double *buf = m->inlet_bufs[0];
    for (int i = 0; i < BUF_SIZE; i++) {
      if (buf[i] != 1.0) {
        fprintf(stderr, "FAIL: offset=%d, buf[%d] = %f (expected 1.0)\n",
                offset, i, buf[i]);
        assert(0);
      }
    }
  }

  free(m);
}

static void test_move_overflow_to_msg_queue(void) {
  reset_all();

  /* Push messages directly to overflow queue */
  for (int i = 0; i < 5; i++) {
    scheduler_msg msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)(100 + i)};
    push_msg(&ctx.overflow_queue, msg, 0);
  }
  assert(ctx.overflow_queue.num_msgs == 5);
  assert(ctx.msg_queue.num_msgs == 0);

  move_overflow();

  assert(ctx.overflow_queue.num_msgs == 0);
  assert(ctx.msg_queue.num_msgs == 5);

  /* Verify messages transferred correctly */
  for (int i = 0; i < 5; i++) {
    scheduler_msg out = pop_msg(&ctx.msg_queue);
    assert(out.tick == (uint64_t)(100 + i));
  }
}

/* ══════════════════════════════════════════════════════════════
   Main
   ══════════════════════════════════════════════════════════════ */

int main(void) {
  init_heap(&scheduler_queue);

  printf("\n=== Layer 1: EventHeap ===\n");
  RUN_TEST(test_heap_insert_pop_order);
  RUN_TEST(test_heap_pop_empty);
  RUN_TEST(test_heap_duplicate_ticks);
  RUN_TEST(test_heap_single_element);
  RUN_TEST(test_heap_grow_past_capacity);
  RUN_TEST(test_heap_large_batch);
  RUN_TEST(test_heap_interleaved_push_pop);

  printf("\n=== Layer 1: msg_queue ===\n");
  RUN_TEST(test_queue_push_pop_single);
  RUN_TEST(test_queue_fill_to_capacity);
  RUN_TEST(test_queue_overflow_drops);
  RUN_TEST(test_queue_wraparound);
  RUN_TEST(test_queue_buffer_offset);
  RUN_TEST(test_queue_num_msgs_tracking);

  printf("\n=== Layer 1.5: Scheduler Logic ===\n");
  RUN_TEST(test_process_events_fires_due);
  RUN_TEST(test_process_events_none_due);
  RUN_TEST(test_process_events_reschedule);
  RUN_TEST(test_defer_quant_alignment);
  RUN_TEST(test_schedule_event_conversion);

  printf("\n=== Layer 2: Message Processing ===\n");
  RUN_TEST(test_pre_on_time_scalar);
  RUN_TEST(test_post_on_time_scalar);
  RUN_TEST(test_pre_late_message);
  RUN_TEST(test_pre_early_message_deferred);
  RUN_TEST(test_pre_post_trig);
  RUN_TEST(test_pre_multiple_messages_ordering);
  RUN_TEST(test_pre_post_full_buffer_coverage);
  RUN_TEST(test_move_overflow_to_msg_queue);

  printf("\n%d/%d tests passed.\n\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
