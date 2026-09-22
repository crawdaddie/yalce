include ../../.env

CC ?= clang

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += `pkg-config --cflags sdl3`

CFLAGS += -Wno-unused-parameter -Wno-unused-function

LDFLAGS   = -shared
LDFLAGS  += `pkg-config --libs sdl3`

ifeq ($(shell uname -s),Darwin)
LDFLAGS += -framework OpenGL
else
LDFLAGS += -lGL
endif

LIB_NAME    = libgfx
DYNAMIC_LIB = $(LIB_NAME).so
BUILD_DIR   = build

C_SOURCES = gfx.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
LOCAL_HEADERS = $(wildcard *.h)

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(LOCAL_HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(DYNAMIC_LIB)

.PHONY: all clean
