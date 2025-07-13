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
CFLAGS := -I./lang 

CFLAGS += -I./gui -I${SDL2_PATH}/include -I${SDL2_PATH}/include/SDL2 -I${SDL2_TTF_PATH}/include

CFLAGS += -I$(READLINE_PREFIX)/include
CFLAGS += -I./lang/backend_llvm
CFLAGS += -I./lang/runtime
CFLAGS += `$(LLVM_CONFIG) --cflags`
CFLAGS += -I`$(LLVM_CONFIG) --includedir`



LANG_CC := clang $(CFLAGS)
LANG_CC += -g

LANG_LD_FLAGS := -lm -framework Accelerate
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline

LANG_CC += -D_USE_BLAS
LANG_CC += -I${OPENBLAS_PATH}/include
LANG_LD_FLAGS += -L${OPENBLAS_PATH}/lib -lopenblas

LANG_CC += -D_USE_OPENMP
LANG_CC += -I${OPENMP_PATH}/include
LANG_LD_FLAGS += -L${OPENMP_PATH}/lib -lomp

# VST Library path
VST_LIB_PATH := ${VST_LIB_PATH}
VST_BUILD_PATH := $(VST_LIB_PATH)/build
LANG_LD_FLAGS += -L$(VST_BUILD_PATH) -lylcvst

LANG_LD_FLAGS +=-Wl,-rpath,@executable_path/../build/gui
# LANG_LD_FLAGS += -L$(BUILD_DIR)/gui -lgui -L${SDL2_PATH}/lib -L${SDL2_TTF_PATH}/lib -lSDL2 -lSDL2_ttf -L${SDL2_GFX_PATH}/lib -lSDL2_gfx -L$(VST_BUILD_PATH)

# VST Library path
# VST_LIB_PATH := ${VST_LIB_PATH}
# VST_BUILD_PATH := $(VST_LIB_PATH)/build
# LANG_LD_FLAGS += -lylcvst

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
	flex -o $(LEX_OUTPUT) $(LEX_FILE)

# Build language object files
$(BUILD_DIR)/%.o: $(LANG_SRC_DIR)/%.c $(YACC_OUTPUT) $(LEX_OUTPUT) | $(BUILD_DIR)
	$(LANG_CC) -c -o $@ $<

# Build the final executable
$(BUILD_DIR)/ylc: $(LANG_OBJS) | engine gui
	$(LANG_CC) -o $@ $(LANG_OBJS) $(LANG_LD_FLAGS)
	otool -L $(BUILD_DIR)/ylc

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

# Jupyter kernel setup and installation
KERNEL_DIR := dev/kernel
VENV_DIR := $(KERNEL_DIR)/.venv

.PHONY: kernel-venv kernel-install kernel-uninstall kernel-clean

# Create a Python virtual environment for the kernel
kernel-venv:
	@echo "Creating virtual environment for YLC kernel..."
	cd $(KERNEL_DIR) && python -m venv .venv
	@echo "Installing kernel dependencies..."
	cd $(KERNEL_DIR) && . .venv/bin/activate && pip install -e .

kernel-install: kernel-venv
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
	rm -rf $(KERNEL_DIR)/*.egg-info
	rm -rf $(KERNEL_DIR)/build
	rm -rf $(KERNEL_DIR)/dist
	rm -rf $(KERNEL_DIR)/__pycache__
	rm -rf $(KERNEL_DIR)/ylc_kernel/__pycache__

# Combined target to build YLC and install the kernel
jupyter: $(BUILD_DIR)/ylc kernel-install
	@echo "YLC and Jupyter kernel setup complete."
