include ../.env
# engine/Makefile

# Detect operating system
UNAME := $(shell uname)

BUILD_DIR := ../build/engine
SRCS := $(wildcard *.c)
SRCS += faust/temper.c
SRCS += faust/vital_rev.c
SRCS += faust/zita_rev.c
SRCS += faust/imp_train.c

# Platform-specific sources and settings
ifeq ($(UNAME), Darwin)
    # macOS specific
    # Include macOS MIDI implementation, exclude Linux MIDI implementation
    SRCS += midi_macosx.c
    SRCS := $(filter-out midi_linux.c, $(SRCS))

    SRCS += vst/vst_node.c
    VST_LIB_PATH := ${VST_LIB_PATH}
    VST_BUILD_PATH := $(VST_LIB_PATH)/build
    LDFLAGS = -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2 -L$(VST_BUILD_PATH) -lylcvst -lstdc++
    FRAMEWORKS = -framework opengl \
                 -framework CoreMIDI \
                 -framework cocoa \
                 -framework CoreAudio \
                 -framework AudioToolbox \
                 -framework CoreFoundation
    COMPILER_OPTIONS = -g -I$(VST_LIB_PATH)/src
    SHARED_LIB_FLAGS = -shared
    SHARED_LIB_EXT = .so
    POST_BUILD = install_name_tool -id @rpath/libyalce_synth.so $(SHARED_LIB_TARGET)
    VST_DEP = vst_lib
else ifeq ($(UNAME), Linux)
    # Linux specific
    # Include Linux MIDI implementation, exclude macOS MIDI implementation
    SRCS += midi_linux.c
    SRCS := $(filter-out midi_macosx.c, $(SRCS))

    # Linux specific
    LDFLAGS = -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2 -lasound -lpthread -lstdc++
    FRAMEWORKS =
    COMPILER_OPTIONS = -g -fPIC
    SHARED_LIB_FLAGS = -shared -fPIC
    SHARED_LIB_EXT = .so
    POST_BUILD =
    VST_DEP =
endif

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
CC = clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth$(SHARED_LIB_EXT)

.PHONY: all clean ylc_bindings vst_lib

all: $(VST_DEP) $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/faust
	mkdir -p $(BUILD_DIR)/vst

# Build the VST library (macOS only)
vst_lib:
ifeq ($(UNAME), Darwin)
	@echo "Building VST library..."
	$(MAKE) -C $(VST_LIB_PATH)
endif

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

ylc_bindings:
	$(MAKE) -C ../tools
	../build/tools/ffi_gen osc.h \
		ext_lib.h \
		envelope.h \
		filter.h \
		node_util.h \
		lfo.h \
		bus.h \
		glue.h \
		faust.h \
		> bindings/Synth.ylc
	cat bindings/_engine_synth_extra.ylc >> bindings/Synth.ylc
	../build/tools/ffi_gen scheduling.h > bindings/Sched.ylc
	../build/tools/ffi_gen midi.h > bindings/MIDI.ylc
	cat bindings/_engine_midi_extra.ylc >> bindings/MIDI.ylc

# Build the shared library
$(SHARED_LIB_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJS)) | $(VST_DEP)
	$(CC) $(SHARED_LIB_FLAGS) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
	$(POST_BUILD)
	$(MAKE) -C ../tools
	$(MAKE) -C . ylc_bindings

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
ifeq ($(UNAME), Darwin)
	@echo "Cleaning VST library..."
	$(MAKE) -C $(VST_LIB_PATH) clean
endif
