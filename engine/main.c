#include "audio_loop.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  init_audio();
  Node *sq = malloc(sizeof(Node));
  sq->state = malloc(sizeof(sq_state));
  sq->perform = (node_perform)sq_perform;
  sq->frame_offset = 0;

  sq->num_ins = 1;
  sq->ins = calloc(1, sizeof(double *));
  sq->ins[0] = calloc(BUF_SIZE, sizeof(double));

  for (int i = 0; i < BUF_SIZE; i++) {
    sq->ins[0][i] = 100.;
  }
  sq->type = OUTPUT;
  sq->next = NULL;

  group_state *gr = calloc(1, sizeof(group_state));
  gr->head = sq;

  Node *group = calloc(1, sizeof(Node));
  group->state = gr;
  group->perform = (node_perform)group_perform;
  group->is_group = true;
  group->num_ins = 0;
  group->ins = NULL;
  group->frame_offset = 0;

  printf("sq %p frame offset %d perform %p\n", sq, sq->frame_offset,
         sq->perform);
  // group_add(gr, sq);

  printf("group %p output %p\n", group, group->output_buf);

  group->is_group = 1;
  group->type = OUTPUT;
  audio_ctx_add(group);
  group->next = NULL;

  while (1) {
  }
  return 0;
}
