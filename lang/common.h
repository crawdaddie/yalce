#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  char *chars;
  int length;
  uint64_t hash;
} ObjString;

uint64_t hash_string(const char *key, int length);
uint64_t hash_key(const char *key);

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

extern int __BREAK_REPL_FOR_GUI_LOOP;
extern void (*break_repl_for_gui_loop_cb)(void); // Changed from int to void

void __set_break_repl_flag(bool f);
void __set_break_repl_cb(void (*cb)(void)); // Proper function pointer type
//
typedef void *(*AllocatorFnType)(size_t size);
typedef void *(*ReAllocatorFnType)(void *p, size_t size);

#define CHARS_EQ(a, b) (strcmp(a, b) == 0)
#endif
