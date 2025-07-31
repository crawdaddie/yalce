#ifndef _LANG_CONFIG_H
#define _LANG_CONFIG_H
#include <stdbool.h>

typedef struct RTConfig {
  bool interactive_mode;
  bool test_mode;
  bool gui_mode;
  bool debug_codegen;
  bool debug_ir;
  const char *base_libs_dir;
  const char *import_current_dir;
  const char *opt_level;

} RTConfig;
extern RTConfig config;

#endif
