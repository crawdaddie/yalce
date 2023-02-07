#include "./channel.c"
#include "audio/delay.c"
#include "audio/out.c"
#include "audio/sq.c"
#include "ctx.c"
#include "graph/graph.c"
#include "parse.c"
#include "sched.c"
#include "scheduling.h"
#include <stdlib.h>

double frame_offset_secs() {
  double clock_start = sched_clock_start();
  double callback_time = sched_get_time();
  double clock_time = timespec_to_secs(get_time()) - clock_start;
  double frame_offset_secs = clock_time - callback_time;
  /* printf("callback time %f clock time %f frame offset %f\n", callback_time,
   */
  /*        clock_time, frame_offset_secs); */
  return frame_offset_secs;
}

static Graph *synth(Graph *tail, double *out) {
  tail = add_after(tail, sq_create(NULL));
  tail = add_after(tail, delay_create(tail->out));
  tail = add_after(tail, replace_out(tail->out, out));
  /* tail = add_after(tail, add_out(tail->out, out)); */
  return tail;
}

static Group create_synth_group(double *out, double frame_offset) {
  Graph *sq = sq_create(NULL);
  Graph *del = delay_create(NULL);
  pipe_graph(sq, del);
  Graph *bus_out = replace_out(NULL, out);
  pipe_graph(del, bus_out);
  return (Group){.head = sq, .tail = bus_out};
}

Group DISPATCH_ADD_SQ_SYNTH_MSG(Graph *node) {
  Group g = create_synth_group(channel_out(0), frame_offset_secs());
  node->_graph = g.head;
  return g;
}

static Graph *delay(Graph *tail, double *out) { return tail; }
void DISPATCH_ADD_DELAY_SYNTH_MSG() {}

void play(void *vargp) {

  Group g = DISPATCH_ADD_SQ_SYNTH_MSG(ctx_graph_head());

  /* Group g = group(ctx_graph_head(), channel_out(0), synth); */
  /* Group d = group(h.tail, channel_out(0), delay); */

  double sequence[7] = {1.0,
                        1.122462048309373,
                        1.189207115002721,
                        1.3348398541700344,
                        1.4983070768766815,
                        1.5874010519681994,
                        1.7817974362806785};

  double pan[3] = {0.0, 0.5, 1.0};
  double octave[3] = {1.0, 2.0, 3.0};

  int time_seq[7] = {1, 2, 3, 1, 1, 4, 1};

  for (int i = 0;; i = (i + 1) % 7) {
    frame_offset_secs();

    set_signal(g.head->in[0], sequence[arc4random_uniform(7)] * 110 *
                                  octave[arc4random_uniform(3)]);

    /* ((sq_data *)g.head->data)->pan = pan[i % 3]; */

    /* ((sq_data *)h.head->data)->pan = pan[(2 - i) % 3]; */

    msleep(500);
    /* set_signal(h.head->in[0], sequence[arc4random_uniform(7)] * 0.5 * 220);
     */

    /* msleep(250); */
  }
}
