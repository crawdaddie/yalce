include .env

BUILD_DIR := build

LLVM := ${LLVM_PATH}
# Debug LLVM path (you can change this to your debug LLVM installation path)
DEBUG_LLVM := ~/projects/llvm-debug
# Use DEBUG_LLVM if the target is debug, otherwise use the default LLVM
ifeq ($(MAKECMDGOALS),debug)
    LLVM := $(DEBUG_LLVM)
endif

LLVM_CONFIG := $(LLVM)/bin/llvm-config

# macOS-specific settings
READLINE_PREFIX := ${READLINE_PREFIX}

LANG_SRC_DIR := lang
LANG_SRCS := $(filter-out $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/lex.yy.c, $(wildcard $(LANG_SRC_DIR)/*.c))

# Separate CFLAGS for include paths
CFLAGS := -I./lang -I./engine

CFLAGS += -I./gui -I${SDL2_PATH}/include -I${SDL2_PATH}/include/SDL2 -I${SDL2_TTF_PATH}/include

CFLAGS += -I$(READLINE_PREFIX)/include
CFLAGS += -I./lang/backend_llvm
CFLAGS += -I./lang/runtime
CFLAGS += `$(LLVM_CONFIG) --cflags`
CFLAGS += -I/opt/homebrew/Cellar/llvm@16/16.0.6_1/include


LANG_CC := clang $(CFLAGS)
LANG_CC += -g

LANG_LD_FLAGS := -L$(BUILD_DIR)/engine -lyalce_synth -lm -framework Accelerate
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline -lSDL2
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline
LANG_LD_FLAGS += -Wl,-rpath,@executable_path/engine

LANG_LD_FLAGS += -L$(BUILD_DIR)/gui -lgui -L${SDL2_PATH}/lib -L${SDL2_TTF_PATH}/lib -lSDL2 -lSDL2_ttf -L${SDL2_GFX_PATH}/lib -lSDL2_gfx

LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/types/*.c)
# Add cor source to LANG_SRCS

ifdef DUMP_AST 
LANG_CC += -DDUMP_AST
endif

ifdef DUMP_AST 
LANG_CC += -DDUMP_AST
endif

LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/backend_llvm/*.c)
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/runtime/*.c)
LANG_CC += -DLLVM_BACKEND
LANG_LD_FLAGS += `$(LLVM_CONFIG) --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`

ifeq ($(MAKECMDGOALS),debug)
  LANG_LD_FLAGS += -lz -lzstd -lc++ -lc++abi -lncurses 
endif

LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/y.tab.h

# Ensure y.tab.c and lex.yy.c are built before any object files that depend on them
$(LANG_OBJS): $(YACC_OUTPUT) $(LEX_OUTPUT)

LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Explicitly add y.tab.o and lex.yy.o to LANG_OBJS
LANG_OBJS += $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o

.PHONY: all clean engine test wasm serve_docs engine_bindings gui cor

all: $(BUILD_DIR)/ylc
debug: all

engine:
	$(MAKE) -C engine

gui:
	@echo "######### MAKE GUI------------"
	mkdir -p $(BUILD_DIR)/gui
	$(MAKE) -C gui

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/backend_llvm
	mkdir -p $(BUILD_DIR)/types
	mkdir -p $(BUILD_DIR)/runtime

# Build lex and yacc output files
$(YACC_OUTPUT): $(YACC_FILE)
	bison --locations -yd $(YACC_FILE) -o $(LANG_SRC_DIR)/y.tab.c

$(LEX_OUTPUT): $(LEX_FILE)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)

# Build language object files
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILD_DIR)
	$(LANG_CC) -c -o $@ $<

# Build the final executable
$(BUILD_DIR)/ylc: $(LANG_OBJS) | engine gui cor
	$(LANG_CC) -o $@ $(LANG_OBJS) $(LANG_LD_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)

test:
	$(MAKE) -C test

test_parse:
	$(MAKE) -C test test_parse

test_typecheck:
	$(MAKE) -C test test_typecheck

test_scripts:
	$(MAKE) -C test test_scripts

wasm:
	./build_wasm.sh

serve_docs:
	python -m http.server -d docs

audio_test:
	$(MAKE) -C engine audio_test

# Jupyter kernel specifics
JUPYTER_DIR := dev/jupyter_kernel
JUPYTER_SRC := $(JUPYTER_DIR)/kernel.c
JUPYTER_OBJ := $(BUILD_DIR)/jupyter_kernel/kernel.o
JUPYTER_TARGET := $(BUILD_DIR)/ylc_kernel

# Jupyter kernel libraries (with macOS specific paths)
JUPYTER_LIBS := -lzmq -ljson-c -luuid -lpthread

# For Apple Silicon Macs (M1/M2)
JUPYTER_LIBS += -L/opt/homebrew/lib -I/opt/homebrew/include

# Jupyter kernel installation directories
KERNEL_SPEC_DIR := $(HOME)/Library/jupyter/kernels/ylc

# Jupyter kernel build
$(JUPYTER_OBJ): $(JUPYTER_SRC) | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/jupyter_kernel
	$(LANG_CC) -c -o $@ $< 

# The Jupyter kernel needs to link to all the same object files as the main executable
# except we don't link to the main.o file if it exists
$(JUPYTER_TARGET): $(JUPYTER_OBJ) $(LANG_OBJS) | engine gui cor
	$(LANG_CC) -o $@ $(JUPYTER_OBJ) $(filter-out $(BUILD_DIR)/main.o, $(LANG_OBJS)) $(JUPYTER_LIBS) $(LANG_LD_FLAGS)
	# $(LANG_CC) -o $@ $(JUPYTER_OBJ) $(filter-out $(BUILD_DIR)/main.o, $(LANG_OBJS)) $(JUPYTER_LIBS) $(LANG_LD_FLAGS) -Wl,-rpath,$(shell pwd)/$(BUILD_DIR)/engine

jupyter: $(JUPYTER_TARGET)

install-jupyter: jupyter
	@echo "Installing Jupyter kernel specification..."
	@mkdir -p $(KERNEL_SPEC_DIR)
	@cp $(JUPYTER_TARGET) $(KERNEL_SPEC_DIR)/
	@echo "{\n  \"argv\": [\"$(KERNEL_SPEC_DIR)/ylc_kernel\", \"-f\", \"{connection_file}\"],\n  \"display_name\": \"YLC Language\",\n  \"language\": \"ylc\"\n}" > $(KERNEL_SPEC_DIR)/kernel.json
	@echo "Kernel specification installed in $(KERNEL_SPEC_DIR)"

