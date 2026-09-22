#ifndef _LANG_COMMON_H
#define _LANG_COMMON_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct Type Type;
typedef struct EscapeMeta EscapeMeta;

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
extern bool __GUI_LOOP_STOP_REQUESTED;

typedef void (*YlcHostTickFn)(void *ctx);
typedef void (*YlcHostLoopFn)(YlcHostTickFn tick, void *ctx);

extern YlcHostLoopFn ylc_host_loop_cb;

void __set_break_repl_flag(bool f);
void __set_break_repl_cb(void (*cb)(void)); // Proper function pointer type
void ylc_host_loop_register(YlcHostLoopFn cb);
void __clear_gui_loop_stop(void);
void __request_gui_loop_stop(void);
bool __gui_loop_should_stop(void);
//
typedef void *(*AllocatorFnType)(size_t size);
typedef void *(*ReAllocatorFnType)(void *p, size_t size);

#define CHARS_EQ(a, b) (strcmp(a, b) == 0)
#endif
