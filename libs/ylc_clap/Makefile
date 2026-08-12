ifneq (,$(wildcard ../../.env))
include ../../.env
endif

CC ?= clang
PKG_CONFIG ?= pkg-config
LLVM_CONFIG ?= /usr/lib/llvm21/bin/llvm-config
BUILD_DIR ?= build
TARGET ?= $(BUILD_DIR)/ylc_script.clap
AUDIO_JIT_ROOT := ../audio_jit
AUDIO_JIT_BITCODE := $(abspath $(AUDIO_JIT_ROOT)/build/osc_kernels.bc)

CFLAGS ?= -O2 -g
CPPFLAGS ?= -DYLC_CLAP_AUDIO_JIT_BITCODE_PATH=\"$(AUDIO_JIT_BITCODE)\"
LDFLAGS ?=
LDLIBS ?= -lm
READLINE_LIBS ?= -lreadline

WARNFLAGS := -Wall -Wextra -Wpedantic
X11_CFLAGS := $(shell $(PKG_CONFIG) --cflags x11 2>/dev/null)
X11_LIBS := $(shell $(PKG_CONFIG) --libs x11 2>/dev/null)
LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags 2>/dev/null)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags 2>/dev/null)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core orcjit native linker bitreader passes analysis 2>/dev/null)
INCLUDES := -isystem include -Isrc -I../../lang -I../../lang/backend_llvm -I../../lang/runtime -I../../engine -I$(AUDIO_JIT_ROOT)
SOURCES := src/plugin.c src/script_runtime.c src/runtime_service.c \
	src/script_jit.c src/orc.c src/lowering.c src/audio_graph.c src/gui.c \
	src/runtime_symbols.c src/scheduler.c src/audio_jit_symbols.c \
	src/debug.c
AUDIO_JIT_SOURCES := $(AUDIO_JIT_ROOT)/audio_jit.c
LANG_ROOT := ../../lang
LANG_SOURCES := $(filter-out \
	$(LANG_ROOT)/main.c \
	$(LANG_ROOT)/backend_llvm/jit.c \
	$(LANG_ROOT)/backend_llvm/orc.c \
	$(LANG_ROOT)/backend_llvm/testing.c, \
	$(wildcard $(LANG_ROOT)/*.c) \
	$(wildcard $(LANG_ROOT)/types/*.c) \
	$(wildcard $(LANG_ROOT)/mir/*.c) \
	$(wildcard $(LANG_ROOT)/llvm/*.c) \
	$(wildcard $(LANG_ROOT)/backend_llvm/*.c) \
	$(wildcard $(LANG_ROOT)/backend_llvm/coroutines/*.c) \
	$(wildcard $(LANG_ROOT)/runtime/*.c))
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
LANG_OBJECTS := $(patsubst $(LANG_ROOT)/%.c,$(BUILD_DIR)/lang/%.o,$(LANG_SOURCES))
OBJECTS += $(LANG_OBJECTS)
AUDIO_JIT_OBJECTS := $(patsubst $(AUDIO_JIT_ROOT)/%.c,$(BUILD_DIR)/audio_jit/%.o,$(AUDIO_JIT_SOURCES))
OBJECTS += $(AUDIO_JIT_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean install validate deps

all: $(TARGET)

deps:
	sh scripts/fetch_deps.sh

$(TARGET): $(OBJECTS) $(AUDIO_JIT_BITCODE)
	$(CC) -shared $(LDFLAGS) $(LLVM_LDFLAGS) -o $@ $^ $(LDLIBS) $(X11_LIBS) $(LLVM_LIBS) $(READLINE_LIBS) -pthread

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -DLLVM_BACKEND $(LLVM_CFLAGS) $(CFLAGS) $(WARNFLAGS) $(INCLUDES) $(X11_CFLAGS) -pthread -fPIC -MMD -MP -c $< -o $@

$(BUILD_DIR)/lang/%.o: $(LANG_ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -DLLVM_BACKEND $(LLVM_CFLAGS) $(CFLAGS) $(WARNFLAGS) $(INCLUDES) -pthread -fPIC -MMD -MP -c $< -o $@

$(BUILD_DIR)/audio_jit/%.o: $(AUDIO_JIT_ROOT)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -DLLVM_BACKEND $(LLVM_CFLAGS) $(CFLAGS) $(WARNFLAGS) -Wno-unused-function -Wno-unused-parameter $(INCLUDES) -pthread -fPIC -MMD -MP -c $< -o $@

$(AUDIO_JIT_BITCODE): $(AUDIO_JIT_ROOT)/osc_kernels.c $(wildcard $(AUDIO_JIT_ROOT)/*.h) $(wildcard ../../engine/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -DLLVM_BACKEND $(LLVM_CFLAGS) $(filter-out -g,$(CFLAGS)) -g0 -O3 -Wno-unused-function -Wno-unused-parameter $(INCLUDES) -emit-llvm -c $< -o $@

install: $(TARGET)
	install -d "$(HOME)/.clap"
	install -m 0755 "$(TARGET)" "$(HOME)/.clap/"

validate: $(TARGET)
	clap-validator validate "$(TARGET)"

clean:
	rm -rf "$(BUILD_DIR)"

-include $(DEPS)
