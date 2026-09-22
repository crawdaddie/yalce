#ifndef _LANG_INPUT_H
#define _LANG_INPUT_H

#include <stdbool.h>
#define INPUT_BUFSIZE 2048

#define YLC_REPL_KEY_BACKSPACE 8
#define YLC_REPL_KEY_ENTER 13
#define YLC_REPL_KEY_LEFT 1000
#define YLC_REPL_KEY_RIGHT 1001
#define YLC_REPL_KEY_UP 1002
#define YLC_REPL_KEY_DOWN 1003

typedef void (*YlcReplLineCb)(const char *input, void *userdata);

char *repl_input(const char *prompt);
char *read_script(const char *filename);

char *get_dirname(const char *path);
char *resolve_relative_path(const char *base_path, const char *relative_path);
char *normalize_path(const char *path);

void init_readline();
void save_history();
void add_completion_item(const char *item, int count);
void ylc_repl_begin(const char *prompt, YlcReplLineCb cb, void *userdata);
void ylc_repl_poll_stdin(void);
void ylc_repl_feed_text(const char *text);
void ylc_repl_feed_key(int key);

const char *get_mod_name_from_path_identifier(const char *str);
#endif
