#include "backend_llvm/jit.h"
#include <stdbool.h>
#ifdef GUI
#include "gui.h"
int main(int argc, char **argv) { return gui_main(argc, argv); }
#else
int main(int argc, char **argv) { return jit(argc, argv); }
#endif
