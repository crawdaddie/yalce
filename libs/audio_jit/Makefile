include ../../.env

CC          = clang
LLVM        = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += -I../../lang/backend_llvm
CFLAGS += -I../../engine
CFLAGS += `$(LLVM_CONFIG) --cflags`

# Suppress warnings from unused Yalce include surface
CFLAGS += -Wno-unused-parameter -Wno-unused-function

LDFLAGS   = -shared
LDFLAGS  += `$(LLVM_CONFIG) --libs core linker passes` -lLLVM-21

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

SOURCES = audio_jit.c compile_synth.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
LOCAL_HEADERS = $(wildcard *.h)
ENGINE_HEADERS = $(wildcard ../../engine/*.h)
ENGINE_LIB = ../../engine/build/libyalce_synth.so

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(LOCAL_HEADERS) $(ENGINE_HEADERS) $(ENGINE_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
