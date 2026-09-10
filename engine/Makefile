# engine/Makefile
include ../.env

BUILD_DIR := ./build
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
COMPILER_OPTIONS := -g -fPIC

LDFLAGS := -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2 -lstdc++

ifeq ($(shell uname -s),Darwin)
    FRAMEWORKS = -framework opengl \
                 -framework CoreMIDI \
                 -framework cocoa \
                 -framework CoreAudio \
                 -framework AudioToolbox \
                 -framework CoreFoundation
else
    FRAMEWORKS = -lasound
endif

COMPILER_OPTIONS += -I../lang/

CC ?= clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean ylc_bindings test

all: $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

ylc_bindings:
	@echo "engine bindings are currently curated in bindings/*.ylc"

# Build the shared library
$(SHARED_LIB_TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
ifeq ($(shell uname -s),Darwin)
	install_name_tool -id @rpath/libyalce_synth.so $@
endif

test: | $(BUILD_DIR)
	$(CC) -g -I. -I../lang/ -o $(BUILD_DIR)/test_scheduling test/test_scheduling.c -lm -lpthread
	$(BUILD_DIR)/test_scheduling

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
