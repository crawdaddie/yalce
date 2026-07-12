#include "./config.h"
#include <stdio.h>

RTConfig ylc_config = {
    .interactive_mode = false,
    .test_mode = false,
    .gui_mode = false,
    .debug_codegen = false,
    .debug_symbols = false,
    .num_input_scripts = 0,
};

void print_config() {
  if (ylc_config.interactive_mode) {
    printf("interactive mode\n");
  }

  if (ylc_config.test_mode) {
    printf("test\n");
  }

  if (ylc_config.debug_codegen) {
    printf("debug codegen\n");
  }

  if (ylc_config.debug_ir) {
    printf("dump LLVM IR\n");
  }

  if (ylc_config.debug_ir_pre) {
    printf("dump LLVM IR pre-opt\n");
  }

  if (ylc_config.dump_mir) {
    printf("dump YLC MIR\n");
  }

  if (ylc_config.debug_symbols) {
    printf("debug mode\n");
  }

  if (ylc_config.verify_ir) {
    printf("Verify LLVM IR\n");
  }

  if (ylc_config.base_libs_dir) {
    printf("base libs dir %s\n", ylc_config.base_libs_dir);
  }

  if (ylc_config.import_current_dir) {
    printf("root for relative imports %s\n", ylc_config.import_current_dir);
  }

  if (ylc_config.opt_level) {
    printf("Optimisation Level %s\n", ylc_config.opt_level);
  }

  if (ylc_config.num_input_scripts) {
    printf("input files %d:\n", ylc_config.num_input_scripts);
    for (int i = 0; i < ylc_config.num_input_scripts; i++) {
      printf("%s, ", ylc_config.input_scripts[i]);
    }
    printf("\n");
  }
}
