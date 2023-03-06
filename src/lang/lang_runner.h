#ifndef _LANG_RUNNER_H
#define _LANG_RUNNER_H
#include "chunk.h"
#include "dbg.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
void run_file(const char *path);
void repl_input(char *input, int bufsize, const char *prompt);
#endif