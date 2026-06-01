/*
 * Tests for scheduling (EventHeap) and audio instructions
 * (audio_instructions_queue, process_msg_queue_pre/post).
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
#include "../audio_instructions.h"
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

int ctx_sample_rate(void) { return 48000; }
double ctx_spf(void) { return 1.0 / 48000.0; }

/* ── Include source files under test ──────────────────────── */

/* scheduling.c — EventHeap, push_event, pop_event, collect_due_events,
   fire_events, schedule_event, defer_quant */
#include "../scheduling.c"

/* audio_instructions.c — process_msg_queue_pre/post, deferred buffer */
#include "../audio_instructions.c"

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

static void reset_msg_queue(audio_instructions_queue *q) {
  memset(q, 0, sizeof(audio_instructions_queue));
}

static void reset_deferred(void) { num_deferred = 0; }

static void reset_all(void) {
  reset_heap();
  reset_msg_queue(&ctx.msg_queue);
  reset_deferred();
  atomic_store(&global_sample_position, 0);
}

/* Helper: process all due events (single-threaded equivalent of the
   scheduler thread's inner loop). */
static void process_scheduler_events(uint64_t current_sample) {
  for (;;) {
    pthread_mutex_lock(&scheduler_mutex);
    int count = collect_due_events(current_sample);
    pthread_mutex_unlock(&scheduler_mutex);
    if (count == 0)
      break;
    fire_events(count);
  }
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
  m->node.state_ptr = &m->graph;

  m->graph.num_inlets = num_inlets;
  m->graph.nodes = m->inlet_nodes;

  for (int i = 0; i < num_inlets; i++) {
    m->graph.inlets[i] = i;
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
  recorded_ticks[recorded_count++] = tick;
}

static void test_heap_insert_pop_order(void) {
  reset_all();
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
  for (int i = 0; i < 100; i++) {
    push_event(record_tick_cb, (void *)(uintptr_t)i, (uint64_t)(100 - i), 0);
  }
  assert(scheduler_queue.size == 100);
  assert(scheduler_queue.capacity >= 100);

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
  for (int i = 0; i < 1000; i++) {
    uint64_t tick = (uint64_t)((i * 7919) % 10000);
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

/* ── audio_instructions_queue tests ──────────────────────── */

static void test_queue_push_pop_single(void) {
  reset_all();
  audio_instructions_queue *q = &ctx.msg_queue;

  audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 100};
  msg.payload.NODE_SET_SCALAR.value = 0.75;

  push_msg(q, msg);
  assert(q->num_msgs == 1);

  audio_instruction out = pop_msg(q);
  assert(out.tick == 100);
  assert(out.type == NODE_SET_SCALAR);
  assert(out.payload.NODE_SET_SCALAR.value == 0.75);
  assert(q->num_msgs == 0);
}

static void test_queue_fill_to_capacity(void) {
  reset_all();
  audio_instructions_queue *q = &ctx.msg_queue;

  for (int i = 0; i < MSG_QUEUE_MAX_SIZE; i++) {
    audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)i};
    push_msg(q, msg);
  }
  assert(q->num_msgs == MSG_QUEUE_MAX_SIZE);

  for (int i = 0; i < MSG_QUEUE_MAX_SIZE; i++) {
    audio_instruction out = pop_msg(q);
    assert(out.tick == (uint64_t)i);
  }
  assert(q->num_msgs == 0);
}

static void test_queue_wraparound(void) {
  reset_all();
  audio_instructions_queue *q = &ctx.msg_queue;

  /* Push 200, pop 200 */
  for (int i = 0; i < 200; i++) {
    audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)i};
    push_msg(q, msg);
  }
  for (int i = 0; i < 200; i++) {
    audio_instruction out = pop_msg(q);
    assert(out.tick == (uint64_t)i);
  }
  assert(q->num_msgs == 0);

  /* Now write_ptr and read_ptr are at 200. Push another 200. */
  for (int i = 0; i < 200; i++) {
    audio_instruction msg = {.type = NODE_SET_SCALAR,
                             .tick = (uint64_t)(1000 + i)};
    push_msg(q, msg);
  }
  assert(q->num_msgs == 200);

  for (int i = 0; i < 200; i++) {
    audio_instruction out = pop_msg(q);
    assert(out.tick == (uint64_t)(1000 + i));
  }
  assert(q->num_msgs == 0);
}

static void test_queue_num_msgs_tracking(void) {
  reset_all();
  audio_instructions_queue *q = &ctx.msg_queue;

  assert(q->num_msgs == 0);

  for (int i = 0; i < 10; i++) {
    audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 0};
    push_msg(q, msg);
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
    push_event(reschedule_cb, ud, 100, tick);
  }
}

static void test_process_events_reschedule(void) {
  reset_all();
  int count = 0;

  push_event(reschedule_cb, &count, 100, 0);

  process_scheduler_events(100);
  assert(count == 1);

  process_scheduler_events(200);
  assert(count == 2);

  process_scheduler_events(300);
  assert(count == 3);
  assert(scheduler_queue.size == 0);
}

static void test_defer_quant_alignment(void) {
  reset_all();
  ctx.sample_rate = 48000;

  /* At sample 0, quant 1.0s -> target = 48000 */
  atomic_store(&global_sample_position, 0);
  defer_quant(1.0, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 48000);

  reset_heap();

  /* At sample 1000, quant 0.5s (24000 samples) -> next boundary = 24000 */
  atomic_store(&global_sample_position, 1000);
  defer_quant(0.5, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 24000);

  reset_heap();

  /* At exact boundary: sample 48000, quant 1.0s -> next boundary = 96000 */
  atomic_store(&global_sample_position, 48000);
  defer_quant(1.0, (DeferQuantCallback)record_tick_cb);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 96000);
}

static void test_schedule_event_conversion(void) {
  reset_all();
  ctx.sample_rate = 48000;

  /* 1 second delay from tick 0 -> tick 48000 */
  int dummy = 42;
  schedule_event(0, 1.0, record_tick_cb, &dummy);
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 48000);

  reset_heap();

  /* NULL userdata -> early return, nothing pushed */
  schedule_event(0, 1.0, record_tick_cb, NULL);
  assert(scheduler_queue.size == 0);
}

/* ══════════════════════════════════════════════════════════════
   LAYER 2: Audio Instruction Processing (pre/post with mock nodes)
   ══════════════════════════════════════════════════════════════ */

static void test_pre_on_time_scalar(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 10};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.5;

  push_msg(&ctx.msg_queue, msg);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < 10; i++) {
    assert(buf[i] == 0.0);
  }
  for (int i = 10; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.5);
  }

  free(m);
}

static void test_post_on_time_scalar(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 10};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.5;

  push_msg(&ctx.msg_queue, msg);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  process_msg_queue_post(0, BUF_SIZE, &ctx.msg_queue, consumed);

  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.5);
  }

  free(m);
}

static void test_pre_late_message(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 50};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.9;

  push_msg(&ctx.msg_queue, msg);

  int consumed = process_msg_queue_pre(100, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

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
     -> tick - current >= frame_count -> deferred locally */
  audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = 1000};
  msg.payload.NODE_SET_SCALAR.target = &m->node;
  msg.payload.NODE_SET_SCALAR.input = 0;
  msg.payload.NODE_SET_SCALAR.value = 0.3;

  push_msg(&ctx.msg_queue, msg);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  /* Buffer should be untouched */
  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.0);
  }

  /* Message should be in the audio-thread-local deferred buffer */
  assert(num_deferred == 1);
  assert(deferred_msgs[0].tick == 1000);

  /* Process again at tick 1000 — deferred message should fire */
  clear_inlet_buf(m, 0);
  reset_msg_queue(&ctx.msg_queue);
  consumed = process_msg_queue_pre(1000, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 0);     /* nothing new in ring buffer */
  assert(num_deferred == 0); /* deferred was consumed */

  for (int i = 0; i < BUF_SIZE; i++) {
    assert(buf[i] == 0.3);
  }

  free(m);
}

static void test_pre_post_trig(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  audio_instruction msg = {.type = NODE_SET_TRIG, .tick = 20};
  msg.payload.NODE_SET_TRIG.target = &m->node;
  msg.payload.NODE_SET_TRIG.input = 0;

  push_msg(&ctx.msg_queue, msg);

  int consumed = process_msg_queue_pre(0, BUF_SIZE, &ctx.msg_queue);
  assert(consumed == 1);

  double *buf = m->inlet_bufs[0];
  for (int i = 0; i < BUF_SIZE; i++) {
    if (i == 20) {
      assert(buf[i] == 1.0);
    } else {
      assert(buf[i] == 0.0);
    }
  }

  process_msg_queue_post(0, BUF_SIZE, &ctx.msg_queue, consumed);
  assert(buf[20] == 0.0);

  free(m);
}

static void test_pre_multiple_messages_ordering(void) {
  reset_all();
  MockSynth *m = create_mock_synth(1);
  clear_inlet_buf(m, 0);

  audio_instruction msg1 = {.type = NODE_SET_SCALAR, .tick = 0};
  msg1.payload.NODE_SET_SCALAR.target = &m->node;
  msg1.payload.NODE_SET_SCALAR.input = 0;
  msg1.payload.NODE_SET_SCALAR.value = 0.5;

  audio_instruction msg2 = {.type = NODE_SET_SCALAR, .tick = 128};
  msg2.payload.NODE_SET_SCALAR.target = &m->node;
  msg2.payload.NODE_SET_SCALAR.input = 0;
  msg2.payload.NODE_SET_SCALAR.value = 0.8;

  push_msg(&ctx.msg_queue, msg1);
  push_msg(&ctx.msg_queue, msg2);

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

  int offsets[] = {0, 1, BUF_SIZE / 4, BUF_SIZE / 2, BUF_SIZE - 1};
  int noffsets = sizeof(offsets) / sizeof(offsets[0]);

  for (int t = 0; t < noffsets; t++) {
    int offset = offsets[t];
    clear_inlet_buf(m, 0);
    reset_msg_queue(&ctx.msg_queue);
    reset_deferred();

    audio_instruction msg = {.type = NODE_SET_SCALAR, .tick = (uint64_t)offset};
    msg.payload.NODE_SET_SCALAR.target = &m->node;
    msg.payload.NODE_SET_SCALAR.input = 0;
    msg.payload.NODE_SET_SCALAR.value = 1.0;

    push_msg(&ctx.msg_queue, msg);

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

/* ══════════════════════════════════════════════════════════════
   LAYER 3: End-to-end scheduler → audio instruction tests
   ══════════════════════════════════════════════════════════════ */

typedef struct close_payload {
  NodeRef target;
  int gate_input;
} close_payload;

static void close_gate(close_payload *p, uint64_t tick) {
  push_msg(&ctx.msg_queue,
           (audio_instruction){NODE_SET_SCALAR,
                               tick,
                               {.NODE_SET_SCALAR = {.target = p->target,
                                                    .input = p->gate_input,
                                                    .value = 0.}}});
  free(p);
}

static void test_schedule_close_gate(void) {
  reset_all();
  ctx.sample_rate = 48000;

  MockSynth *m = create_mock_synth(2);
  uint64_t note_on_tick = 1000;
  double dur = 0.5; /* 0.5s = 24000 samples */
  int gate_in = 1;

  /* Schedule the close_gate event, same as play_node_dur does */
  close_payload *cp = malloc(sizeof(close_payload));
  *cp = (close_payload){.target = &m->node, .gate_input = gate_in};
  schedule_event(note_on_tick, dur, (SchedulerCallback)close_gate, cp);

  /* Expected fire tick: 1000 + 24000 = 25000 */
  assert(scheduler_queue.size == 1);
  assert(scheduler_queue.events[0].tick == 25000);

  /* Too early — nothing should fire */
  process_scheduler_events(20000);
  assert(ctx.msg_queue.num_msgs == 0);

  /* Advance past the target tick — close_gate fires, pushes to queue */
  process_scheduler_events(25000);
  assert(ctx.msg_queue.num_msgs == 1);

  audio_instruction out = pop_msg(&ctx.msg_queue);
  assert(out.type == NODE_SET_SCALAR);
  assert(out.tick == 25000);
  assert(out.payload.NODE_SET_SCALAR.target == &m->node);
  assert(out.payload.NODE_SET_SCALAR.input == gate_in);
  assert(out.payload.NODE_SET_SCALAR.value == 0.0);

  assert(scheduler_queue.size == 0);
  free(m);
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

  printf("\n=== Layer 1: audio_instructions_queue ===\n");
  RUN_TEST(test_queue_push_pop_single);
  RUN_TEST(test_queue_fill_to_capacity);
  RUN_TEST(test_queue_wraparound);
  RUN_TEST(test_queue_num_msgs_tracking);

  printf("\n=== Layer 1.5: Scheduler Logic ===\n");
  RUN_TEST(test_process_events_fires_due);
  RUN_TEST(test_process_events_none_due);
  RUN_TEST(test_process_events_reschedule);
  RUN_TEST(test_defer_quant_alignment);
  RUN_TEST(test_schedule_event_conversion);

  printf("\n=== Layer 2: Audio Instruction Processing ===\n");
  RUN_TEST(test_pre_on_time_scalar);
  RUN_TEST(test_post_on_time_scalar);
  RUN_TEST(test_pre_late_message);
  RUN_TEST(test_pre_early_message_deferred);
  RUN_TEST(test_pre_post_trig);
  RUN_TEST(test_pre_multiple_messages_ordering);
  RUN_TEST(test_pre_post_full_buffer_coverage);

  printf("\n=== Layer 3: Scheduler → Audio Instructions ===\n");
  RUN_TEST(test_schedule_close_gate);

  printf("\n%d/%d tests passed.\n\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
