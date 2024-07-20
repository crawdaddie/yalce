#ifndef _LANG_INPUT_H
#define _LANG_INPUT_H

#define INPUT_BUFSIZE 2048
#include <stdio.h>
char *repl_input(const char *prompt);
char *read_script(const char *filename);

char *get_dirname(const char *path);

void init_readline();
void add_completion_item(const char *item, int count);
#endif
