include ../.env
# engine/Makefile

BUILD_DIR := ../build/engine
SRCS := $(wildcard *.c)
SRCS += faust/temper.c
SRCS += faust/vital_rev.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
CLAP_SDK_PATH := ${CLAP_PATH}
LDFLAGS = -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2
FRAMEWORKS = -framework opengl \
						 -framework CoreMIDI \
						 -framework cocoa \
						 -framework CoreAudio \
						 -framework AudioToolbox \
						 -framework CoreFoundation

COMPILER_OPTIONS = -g -I$(CLAP_SDK_PATH)/include
CC = clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean ylc_bindings

all: $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/faust

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

ylc_bindings:
	$(MAKE) -C ../tools
	../build/tools/ffi_gen osc.h ext_lib.h envelope.h filter.h node_util.h lfo.h bus.h glue.h faust.h > bindings/Synth.ylc
	cat bindings/_engine_synth_extra.ylc >> bindings/Synth.ylc
	../build/tools/ffi_gen scheduling.h > bindings/Sched.ylc
	../build/tools/ffi_gen midi.h > bindings/MIDI.ylc
	cat bindings/_engine_midi_extra.ylc >> bindings/MIDI.ylc
	# ../build/tools/ffi_gen clap_node.h > bindings/ClapPlugin.ylc

# Build the shared library
$(SHARED_LIB_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
	install_name_tool -id @rpath/libyalce_synth.so $@
	$(MAKE) -C ../tools;
	$(MAKE) -C . ylc_bindings

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
