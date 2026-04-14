include ../../.env

CC ?= clang
# LLVM        = ${LLVM_PATH}
# LLVM_CONFIG = $(LLVM)/bin/llvm-config

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += -I../../engine
CFLAGS += `pkg-config --cflags sdl3`

# Suppress warnings from unused Yalce include surface
CFLAGS += -Wno-unused-parameter -Wno-unused-function

LDFLAGS   = -shared
LDFLAGS  += `pkg-config --libs sdl3`

LIB_NAME    = libgui
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

SOURCES = gui.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
LOCAL_HEADERS = $(wildcard *.h)

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
