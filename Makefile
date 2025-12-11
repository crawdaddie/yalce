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

LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/types/*.c)
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/backend_llvm/*.c)
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/backend_llvm/coroutines/*.c)
LANG_SRCS += $(wildcard $(LANG_SRC_DIR)/runtime/*.c)

# C++ sources for coroutine pass integration
LANG_CPP_SRCS := $(wildcard $(LANG_SRC_DIR)/backend_llvm/*.cpp)
CFLAGS := -I./lang 
CFLAGS += -I./gui -I${SDL2_PATH}/include -I${SDL2_PATH}/include/SDL2 -I${SDL2_TTF_PATH}/include

CFLAGS += -I$(READLINE_PREFIX)/include
CFLAGS += -I./lang/backend_llvm
CFLAGS += -I./lang/runtime
CFLAGS += `$(LLVM_CONFIG) --cflags`
CFLAGS += -I`$(LLVM_CONFIG) --includedir`


LANG_CC := clang $(CFLAGS)
LANG_CC += -g
LANG_CC += -Wall

# C++ compiler for coroutine passes
LANG_CXX := clang++ $(CFLAGS)
LANG_CXX += -g
LANG_CXX += -Wall
LANG_CXX += -std=c++17

LANG_LD_FLAGS := -lm
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline
LANG_LD_FLAGS += -lc++  # C++ standard library

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
LANG_OBJS += $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o
LANG_CPP_OBJS := $(LANG_CPP_SRCS:$(LANG_SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
LANG_OBJS += $(LANG_CPP_OBJS)

.PHONY: all clean engine test wasm serve_docs engine_bindings gui cor

all: $(BUILD_DIR)/ylc

debug: all

engine:
	@echo "######### MAKE ENGINE------------"
	$(MAKE) -C engine

gui:
	@echo "######### MAKE GUI------------"
	$(MAKE) -C gui

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/backend_llvm
	mkdir -p $(BUILD_DIR)/backend_llvm/coroutines
	mkdir -p $(BUILD_DIR)/types
	mkdir -p $(BUILD_DIR)/runtime

# Define Linux-specific YACC flags
ifeq ($(shell uname -s),Linux)
    LINUX_YACC_FLAGS = -Wno-yacc
else
    LINUX_YACC_FLAGS =
endif
# Build lex and yacc output files
$(YACC_OUTPUT): $(YACC_FILE)
	bison --locations -yd $(YACC_FILE) -o $(LANG_SRC_DIR)/y.tab.c

$(LEX_OUTPUT): $(LEX_FILE)
	flex --header-file=$(LANG_SRC_DIR)/lex.yy.h -o $(LEX_OUTPUT) $(LEX_FILE)

# Build language object files
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILD_DIR)
	$(LANG_CC) -c -o $@ $<

# Build C++ object files for coroutine passes
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(LANG_CXX) -c -o $@ $<

# Build the final executable (use C++ linker since we have C++ objects)
$(BUILD_DIR)/ylc: $(LANG_OBJS) | engine gui
	$(LANG_CXX) -o $@ $(LANG_OBJS) $(LANG_LD_FLAGS)
ifeq ($(shell uname -s),Darwin)
	otool -L $(BUILD_DIR)/ylc
else
	ldd $(BUILD_DIR)/ylc || true
endif

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT)
	$(MAKE) -C engine clean
	$(MAKE) -C gui clean

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
	cp lang/backend_wasm/wasm.js docs/web/wasm.js

serve_docs:
	$(MAKE) -C docs dev

audio_test:
	$(MAKE) -C engine audio_test

# Jupyter kernel setup and installation
KERNEL_DIR := dev/kernel
VENV_DIR := $(KERNEL_DIR)/.venv

.PHONY: kernel-venv kernel-install kernel-uninstall kernel-clean wasm

# Create a Python virtual environment for the kernel
kernel-venv:
	@echo "Creating virtual environment for YLC kernel..."
	cd $(KERNEL_DIR) && python -m venv .venv
	@echo "Installing kernel dependencies..."
	cd $(KERNEL_DIR) && . .venv/bin/activate && pip install -e .

kernel-install: kernel-venv all
	@echo "Installing YLC kernel in Jupyter..."
	cd $(KERNEL_DIR) && . .venv/bin/activate && python -m ylc_kernel.install install --ylc-path $(shell pwd)/$(BUILD_DIR)/ylc --user
	@echo "YLC kernel installed successfully. Use 'jupyter lab' to start Jupyter."

# Uninstall the YLC kernel from Jupyter
kernel-uninstall:
	@echo "Uninstalling YLC kernel from Jupyter..."
	cd $(KERNEL_DIR) && . .venv/bin/activate && python -m ylc_kernel.install uninstall

# Clean up kernel installation files
kernel-clean: kernel-uninstall
	@echo "Cleaning up kernel files..."
	rm -rf $(VENV_DIR)
	rm -rf $(KERNEL_DIR)/*

# Combined target to build YLC and install the kernel
jupyter: $(BUILD_DIR)/ylc kernel-install
	@echo "YLC and Jupyter kernel setup complete."
