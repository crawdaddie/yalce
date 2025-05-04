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

# Original dynamic linking flags
LANG_LD_FLAGS := -L$(BUILD_DIR)/engine -lyalce_synth -lm -framework Accelerate
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline -lSDL2
LANG_LD_FLAGS += -L$(READLINE_PREFIX)/lib -lreadline
LANG_LD_FLAGS += -Wl

LANG_LD_FLAGS += -L$(BUILD_DIR)/gui -lgui -L${SDL2_PATH}/lib -L${SDL2_TTF_PATH}/lib -lSDL2 -lSDL2_ttf -L${SDL2_GFX_PATH}/lib -lSDL2_gfx

# Static linking flags - used for the static target (macOS compatible)
# Use linking flags that force inclusion of all symbols from the library
STATIC_LD_FLAGS := -Wl,-force_load,$(BUILD_DIR)/engine/libyalce_synth.a -lm -framework Accelerate
# Define SOUNDIO_STATIC_LIBRARY for static linking with libsoundio
STATIC_LD_FLAGS += -DSOUNDIO_STATIC_LIBRARY
STATIC_LD_FLAGS += /opt/homebrew/lib/libsoundio.a
STATIC_LD_FLAGS += /opt/homebrew/lib/libsndfile.a
STATIC_LD_FLAGS += /opt/homebrew/lib/libfftw3.a
# STATIC_LD_FLAGS += /opt/homebrew/lib/libxml2.a
#
# Add all dependencies needed by libyalce_synth.a
# STATIC_LD_FLAGS += -L/opt/homebrew/lib -lsoundio -lsndfile -lfftw3 -ldl -lxml2
STATIC_LD_FLAGS += -framework OpenGL -framework CoreMIDI -framework Cocoa 

# For readline, use the static version + necessary termcap functions from ncurses
STATIC_LD_FLAGS += $(READLINE_PREFIX)/lib/libreadline.a -lncurses
STATIC_LD_FLAGS += -static-libstdc++

# For GUI libraries - use static versions and link dynamically to Apple frameworks
STATIC_LD_FLAGS += $(BUILD_DIR)/gui/libgui.a
STATIC_LD_FLAGS += ${SDL2_PATH}/lib/libSDL2.a
STATIC_LD_FLAGS += ${SDL2_TTF_PATH}/lib/libSDL2_ttf.a
STATIC_LD_FLAGS += ${SDL2_GFX_PATH}/lib/libSDL2_gfx.a

# Add all frameworks needed by SDL2 on macOS
STATIC_LD_FLAGS += -framework CoreFoundation -framework CoreAudio -framework AudioToolbox 
STATIC_LD_FLAGS += -framework CoreVideo -framework CoreGraphics -framework CoreHaptics
STATIC_LD_FLAGS += -framework GameController -framework Metal -framework MetalKit
STATIC_LD_FLAGS += -framework AppKit -framework Carbon -framework IOKit
STATIC_LD_FLAGS += -framework ForceFeedback -framework CoreServices
STATIC_LD_FLAGS += -framework AudioUnit -framework OpenGL -framework Cocoa
STATIC_LD_FLAGS += -framework Foundation -framework QuartzCore
# Additional libraries that might be needed
STATIC_LD_FLAGS += -liconv -lobjc

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

# Dynamic LLVM flags
LANG_LD_FLAGS += `$(LLVM_CONFIG) --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`

# Static LLVM flags - use dynamic LLVM library to avoid symbol conflicts
STATIC_LD_FLAGS += `$(LLVM_CONFIG) --ldflags --libs`

# Additional dependencies that might be needed (handle FreeType from SDL2_ttf)
STATIC_LD_FLAGS += -L/opt/homebrew/opt/freetype/lib -lfreetype
STATIC_LD_FLAGS += -L/opt/homebrew/opt/harfbuzz/lib -lharfbuzz

ifeq ($(MAKECMDGOALS),debug)
  LANG_LD_FLAGS += -lz -lzstd -lc++ -lc++abi -lncurses 
  STATIC_LD_FLAGS += -lz -lzstd -lc++ -lc++abi -lncurses
endif

LEX_FILE := $(LANG_SRC_DIR)/lex.l
YACC_FILE := $(LANG_SRC_DIR)/parser.y
LEX_OUTPUT := $(LANG_SRC_DIR)/lex.yy.c
YACC_OUTPUT := $(LANG_SRC_DIR)/y.tab.c $(LANG_SRC_DIR)/y.tab.h

# Ensure y.tab.c and lex.yy.c are built before any object files that depend on them
$(LANG_OBJS): $(YACC_OUTPUT) $(LEX_OUTPUT)

LANG_SRCS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(LANG_SRC_DIR)/%.c)
LANG_OBJS := $(LANG_SRCS:$(LANG_SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Explicitly add y.tab.o and lex.yy.o to LANG_OBJS
LANG_OBJS += $(BUILD_DIR)/y.tab.o $(BUILD_DIR)/lex.yy.o

.PHONY: all clean engine engine_static test wasm serve_docs engine_bindings gui cor static

all: $(BUILD_DIR)/ylc
debug: all
static: $(BUILD_DIR)/ylc.static

engine:
	@echo "Building engine libraries (shared and static)..."
	$(MAKE) -C engine all

engine_static:
	@echo "Building engine static library only..."
	$(MAKE) -C engine static

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

# Build the regular executable
$(BUILD_DIR)/ylc: $(LANG_OBJS) | engine gui cor
	$(LANG_CC) -o $@ $(LANG_OBJS) $(LANG_LD_FLAGS)

# Build the mostly-static executable (macOS compatible)
$(BUILD_DIR)/ylc.static: $(LANG_OBJS) | engine_static gui cor
	@echo "Building static version of YLC..."
	@echo "Using static libraries where possible with dynamic frameworks"
	# Link with static libraries where possible, dynamic for system frameworks
	$(LANG_CC) -o $@ $(LANG_OBJS) $(STATIC_LD_FLAGS)
	# Make the binary smaller
	strip -x $@
	# Print information about the linked libraries
	@echo "Library dependencies for $(BUILD_DIR)/ylc.static:"
	@otool -L $@
	@echo "Static build complete!"

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

.PHONY: kernel-venv kernel-install kernel-uninstall kernel-clean static

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
