#ifndef _LANG_CONFIG_H
#define _LANG_CONFIG_H
#include <stdbool.h>

typedef struct RTConfig {
  bool interactive_mode;
  bool test_mode;
  bool gui_mode;
  bool debug_codegen;
  bool debug_ir;
  bool debug_ir_pre;
  bool dump_mir;
  bool debug_symbols;
  bool verify_ir;
  const char *base_libs_dir;
  const char *import_current_dir;
  const char *opt_level;
  const char **input_scripts;
  int num_input_scripts;

} RTConfig;
extern RTConfig ylc_config;

void print_config();
#endif
