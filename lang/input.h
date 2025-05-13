#ifndef _LANG_INPUT_H
#define _LANG_INPUT_H

#include <stdbool.h>
#define INPUT_BUFSIZE 2048
char *repl_input(const char *prompt);
char *read_script(const char *filename);

char *get_dirname(const char *path);
char *resolve_relative_path(const char *base_path, const char *relative_path);
char *normalize_path(const char *path);

void init_readline();
void add_completion_item(const char *item, int count);

const char *get_mod_name_from_path_identifier(const char *str);
#endif
