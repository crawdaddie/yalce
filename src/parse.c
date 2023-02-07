#ifndef _PARSE_H
#define _PARSE_H
#include "graph/graph.c"
enum SYMBOLS {
  OPEN_BRACKETS = '(',
  CLOSE_BRACKETS = ')',
  SPACE = ' ',
  OPEN_CURLY_BRACKETS = '{',
  CLOSE_CURLY_BRACKETS = '{',
};

Group *parse_synth(char *def, Group *g) {
  Graph *head, *tail;
  int len = strlen(def);

  if (!g) {
    g = malloc(sizeof(Group));
    head = g->head;
    tail = g->tail;
  }

  if (strlen(def) == 0) {
    return g;
  }
  char *word, *brkt;
  for (word = strtok_r(def, " \n", &brkt); word;
       word = strtok_r(NULL, " \n", &brkt)) {

    printf("w: '%s'\n", word);
  }

  return g;
}

#endif
