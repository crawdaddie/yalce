#ifndef _LANG_INPUT_H
#define _LANG_INPUT_H

#define INPUT_BUFSIZE 2048
void repl_input(char *input, int bufsize, const char *prompt);
char *read_script(const char *filename);
#endif
