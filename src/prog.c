#include "./channel.c"
#include "audio/delay.c"
#include "audio/out.c"
#include "audio/sq.c"
#include "ctx.c"
#include "graph/graph.c"
#include "scheduling.h"
#include <stdlib.h>

static Graph *synth(Graph *tail, double *out) {
  tail = add_after(tail, sq_create(NULL));
  /* Graph *sq2 = add_after(tail, sq_create(channel_out(0))); */
  tail = add_after(tail, add_out(tail->out, out));
  return tail;
}
static Graph *delay(Graph *tail, double *out) {
  tail = add_after(tail, delay_create(out));
  tail = add_after(tail, replace_out(tail->out, out));
  return tail;
}

void play() {
  Group g = group(ctx_graph_head(), channel_out(0), synth);
  printf("%#08x %#08x\n", g.head, g.tail);

  /* Graph *h = ctx_set_head(sq_create(channel_out(0))); */

  Group h = group(g.tail, channel_out(0), synth);
  Group d = group(h.tail, channel_out(0), delay);

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
    set_signal(g.head->in[0], sequence[arc4random_uniform(7)] * 220 *
                                  octave[arc4random_uniform(3)]);

    /* ((sq_data *)g.head->data)->pan = pan[i % 3]; */

    msleep(250);
    set_signal(h.head->in[0], sequence[arc4random_uniform(7)] * 0.5 * 220);

    msleep(250);
  }
}
