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

void process_token(char *token, Group *group) { printf("w: '%s'\n", token); }
static char separators[] = " ,\n";

/*
 * sq -> delay -> out()
 *
 * */

Group *parse_synth_(char *def, Group *g) {
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
  char *word, *rest;
  for (word = strtok_r(def, separators, &rest); word;
       word = strtok_r(NULL, separators, &rest)) {
    printf("w: '%s' rest: '%s'\n", word, rest);
    /* process_token(word, g); */
  }

  return g;
}

Group *parse_synth(char *def, Group *g) {
  parse_string(def);
  return g;
}

#endif
