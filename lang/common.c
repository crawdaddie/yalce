#include "common.h"
#include <stdbool.h>

// Return 64-bit FNV-1a hash for key (known length rather than null-terminated).
// See description: https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t hash_string(const char *key, int length) {
  uint64_t hash = FNV_OFFSET;
  for (int i = 0; i < length; i++) {
    const char *p = key + i;
    hash ^= (uint64_t)(unsigned char)(*p);
    hash *= FNV_PRIME;
  }
  return hash;
}

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t hash_key(const char *key) {
  uint64_t hash = FNV_OFFSET;
  for (const char *p = key; *p; p++) {
    hash ^= (uint64_t)(unsigned char)(*p);
    hash *= FNV_PRIME;
  }
  return hash;
}

int __BREAK_REPL_FOR_GUI_LOOP = false;
void (*break_repl_for_gui_loop_cb)(void) = NULL; // Changed from int to void

void __set_break_repl_flag(bool f) {
  printf("setting break repl flag %d\n", f);
  __BREAK_REPL_FOR_GUI_LOOP = f;
  printf("flag: %d, callback: %p\n", __BREAK_REPL_FOR_GUI_LOOP,
         break_repl_for_gui_loop_cb);
}

void __set_break_repl_cb(void (*cb)(void)) {
  printf("setting break repl callback to %p\n", cb);
  break_repl_for_gui_loop_cb = cb;
}
