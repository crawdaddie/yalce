// #include "backend_llvm/jit.h"
#include "./backend_llvm/orc.h"
#include "config.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//
// int main(int argc, char **argv) {
//   pthread_t jit_thread;
//   int jit_result;
//   return jit(argc, argv);
// }
//
//
void parse_args(int argc, char **argv) {
  int arg_counter = 1;
  ylc_config.base_libs_dir = getenv("YLC_BASE_DIR");
  int scripts = 0;
  while (arg_counter < argc) {
    if (strcmp(argv[arg_counter], "-i") == 0) {
      ylc_config.interactive_mode = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-codegen") == 0) {
      ylc_config.dump_ir = true;
      ylc_config.debug_codegen = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-ir") == 0) {
      ylc_config.dump_ir = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-ir-pre") == 0) {
      ylc_config.dump_ir_pre = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--dump-mir") == 0) {
      ylc_config.dump_mir = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--test") == 0) {
      // run top-level tests for input module
      ylc_config.test_mode = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--base") == 0) {
      arg_counter++;
      ylc_config.base_libs_dir = argv[arg_counter];
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O0") == 0) {
      ylc_config.opt_level = "default<O0>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O1") == 0) {
      ylc_config.opt_level = "default<O1>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O2") == 0) {
      ylc_config.opt_level = "default<O2>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O3") == 0) {
      ylc_config.opt_level = "default<O3>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--verify-ir") == 0) {
      ylc_config.verify_ir = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--perceus-rc") == 0) {
      ylc_config.perceus_rc = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-g") == 0) {
      ylc_config.debug_symbols = true;
      arg_counter++;
    } else {
      ylc_config.input_scripts = argv + arg_counter;
      ylc_config.num_input_scripts = argc - arg_counter;
      scripts = 1;
      break;
    }
  }

  if (argc == 1 || !scripts) {
    ylc_config.interactive_mode = true;
  }
}

int main(int argc, char **argv) {
  pthread_t jit_thread;
  int jit_result;
  parse_args(argc, argv);
  // print_config();
  return orcjit(argc, argv);
}
