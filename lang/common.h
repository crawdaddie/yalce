#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  char *chars;
  int length;
  uint64_t hash;
} ObjString;

uint64_t hash_string(const char *key, int length);
uint64_t hash_key(const char *key);
#include <stdio.h>

#define TRY_MSG(expr, msg)                                                     \
  ({                                                                           \
    typeof(expr) _result = (expr);                                             \
    if (!_result) {                                                            \
      if (msg) {                                                               \
        fprintf(stderr, "%s\n", msg);                                          \
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                        \
      }                                                                        \
      return NULL;                                                             \
    }                                                                          \
    _result;                                                                   \
  })

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

extern int __BREAK_REPL_FOR_GUI_LOOP;
extern void (*break_repl_for_gui_loop_cb)(void); // Changed from int to void

void __set_break_repl_flag(bool f);
void __set_break_repl_cb(void (*cb)(void)); // Proper function pointer type
#endif
