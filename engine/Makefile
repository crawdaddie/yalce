# engine/Makefile
include ../.env

BUILD_DIR := ./build
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
CONFIG_OBJ := $(BUILD_DIR)/ylc_config.o
OBJS += $(CONFIG_OBJ)
COMPILER_OPTIONS := -g -fPIC

ifeq ($(ASAN),1)
    COMPILER_OPTIONS += -fsanitize=address -fno-omit-frame-pointer \
                        -DYLC_CONFIG_WEAK
endif

ifeq ($(TSAN),1)
    COMPILER_OPTIONS += -fsanitize=thread -fno-omit-frame-pointer
endif

LDFLAGS := -lsoundio -lm -lsndfile -lfftw3 -lxml2

ifeq ($(ASAN),1)
    LDFLAGS += -fsanitize=address
endif

ifeq ($(TSAN),1)
    LDFLAGS += -fsanitize=thread
endif

ifeq ($(shell uname -s),Darwin)
    LDFLAGS += -lpthread
    FRAMEWORKS = -framework opengl \
                 -framework CoreMIDI \
                 -framework cocoa \
                 -framework CoreAudio \
                 -framework AudioToolbox \
                 -framework CoreFoundation
else
    LDFLAGS += -ldl -lstdc++ -lpthread
    FRAMEWORKS = -lasound
endif

COMPILER_OPTIONS += -I../lang/ -I$(LIBXML2_PATH)/include/libxml2

CC ?= clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean ylc_bindings test

all: $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

$(CONFIG_OBJ): ../lang/config.c ../lang/config.h | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

ylc_bindings:
	@echo "engine bindings are currently curated in bindings/*.ylc"

# Build the shared library
$(SHARED_LIB_TARGET): $(OBJS)
	$(CC) $(if $(filter Darwin,$(shell uname -s)),-dynamiclib,-shared) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
ifeq ($(shell uname -s),Darwin)
	install_name_tool -id @rpath/libyalce_synth.so $@
endif

test: | $(BUILD_DIR)
	$(CC) -g -I. -I../lang/ -o $(BUILD_DIR)/test_scheduling test/test_scheduling.c -lm -lpthread
	$(BUILD_DIR)/test_scheduling
	$(CC) -g -I. -o $(BUILD_DIR)/test_audio_loop_utils \
		test/test_audio_loop_utils.c audio_loop_utils.c -lsoundio -lm
	$(BUILD_DIR)/test_audio_loop_utils

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
