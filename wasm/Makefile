# Builds the browser-facing YLC wasm module (docs/web/jit.wasm).
#
# Unlike the legacy build_wasm.sh (which lowered the AST directly), this
# Makefile compiles wasm/main.c — a fresh entry point that reuses the
# lang/ frontend (lexer, parser, type inference, escape analysis, MIR
# builder + passes) and lowers the resulting MirProgram straight to a
# wasm binary. No LLVM headers or libraries are linked.
#
# Usage:
#   make -C wasm                 # build docs/web/jit.wasm
#   make -C wasm clean
#   WASI_SDK_PATH=/opt/wasi-sdk-33 make -C wasm

# ---------------------------------------------------------------------------
# Toolchain (wasi-sdk-33+; clang 22, self-contained wasm-ld + sysroot)
# ---------------------------------------------------------------------------
WASI_SDK_PATH ?= /opt/wasi-sdk-33.0-x86_64-linux
WASI_PREFIX   := $(WASI_SDK_PATH)/bin
CC            := $(WASI_PREFIX)/clang

# wasm32-wasip1 is the non-deprecated name for the classic WASI preview.
# If your wasi-sdk is older and doesn't know wasip1, override TARGET:
#   make -C wasm TARGET=wasm32-wasi
TARGET        ?= wasm32-wasip1
WASI_SYSROOT  := $(WASI_SDK_PATH)/share/wasi-sysroot

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
ROOT_DIR    := ..
LANG_DIR     = $(ROOT_DIR)/lang
BUILD_DIR   := build
WEB_DIR     := $(ROOT_DIR)/docs/web
OUTPUT      := $(WEB_DIR)/jit.wasm

# ---------------------------------------------------------------------------
# Sources
#
# Same frontend surface as the old build_wasm.sh, minus the legacy
# backend_wasm/ AST lowerer (wasm/main.c replaces it) and minus
# lang/llvm/lowering.c (the LLVM dependency we're dropping). input.c is
# excluded too: parse.c references get_dirname/normalize_path/etc. for
# file imports, which the browser host doesn't use — we stub them in
# wasm/main.c via the wasm-allowed-undefined link flag, matching the
# env stubs in docs/web/wasm.js.
# ---------------------------------------------------------------------------
LANG_SRCS := \
	$(wildcard $(LANG_DIR)/*.c) \
	$(wildcard $(LANG_DIR)/mir/*.c) \
	$(wildcard $(LANG_DIR)/types/*.c)

# Exclude files that pull in LLVM or host-only I/O / legacy wasm backend.
EXCLUDE_PATS := \
	$(LANG_DIR)/main.c \
	$(LANG_DIR)/input.c \
	$(LANG_DIR)/synths.c \
	$(LANG_DIR)/ylc_stdlib.c \
	$(LANG_DIR)/llvm/lowering.c \
	$(LANG_DIR)/backend_wasm/wasm.c \
	$(LANG_DIR)/backend_wasm/codegen_wasm.c \
	$(LANG_DIR)/backend_wasm/util.c

LANG_SRCS := $(filter-out $(EXCLUDE_PATS),$(LANG_SRCS))

WASM_SRCS := main.c
SRCS      = $(WASM_SRCS) $(LANG_SRCS)

OBJS := $(patsubst $(LANG_DIR)/%.c,$(BUILD_DIR)/%.o,$(LANG_SRCS))
OBJS += $(patsubst %.c,$(BUILD_DIR)/%.o,$(WASM_SRCS))

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------
INCLUDES := \
	-I$(LANG_DIR) \
	-I$(WASI_SYSROOT)/include

CFLAGS  := --target=$(TARGET) --sysroot=$(WASI_SYSROOT)
CFLAGS  += -O2 -g -Wall -Wno-unused-parameter -Wno-unused-function
CFLAGS  += $(INCLUDES)
CFLAGS  += -D_WASI_EMULATED_MANTISSA
# Strip ANSI escape codes from MIR dumps so they render as plain text in
# the browser terminal element (see lang/format_utils.h).
CFLAGS  += -DYLC_NO_COLOR

LDFLAGS := -nostartfiles -Wl,--no-entry -Wl,--allow-undefined
# Host-visible exports. malloc/free are required so docs/web/wasm.js can
# copy strings/module bytes in and out of linear memory; the ylc_wasm_*
# entry points are the compiler surface defined in wasm/main.c.
EXPORTS  := malloc free ylc_wasm_init ylc_wasm_compile ylc_wasm_dump_mir
LDFLAGS += $(foreach e,$(EXPORTS),-Wl,--export=$(e))

# ---------------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------------
.PHONY: all clean install

all: $(OUTPUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/mir
	mkdir -p $(BUILD_DIR)/types

# wasm/main.c lives in this dir (no $(LANG_DIR)/ prefix).
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(LANG_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT): $(OBJS) | $(WEB_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)
	@echo "built $(OUTPUT)"

$(WEB_DIR):
	mkdir -p $(WEB_DIR)

# Convenience target also copied from the root Makefile's `wasm` recipe.
install: $(OUTPUT)
	cp -f $(OUTPUT) $(WEB_DIR)/jit.wasm

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT)
