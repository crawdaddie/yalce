include ../.env
# engine/Makefile

BUILD_DIR := ../build/engine
SRCS := $(wildcard *.c)
SRCS += faust/temper.c
SRCS += faust/vital_rev.c
SRCS += faust/zita_rev.c
SRCS += faust/imp_train.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
CLAP_SDK_PATH := ${CLAP_PATH}
LDFLAGS = -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2
FRAMEWORKS = -framework opengl \
             -framework CoreMIDI \
             -framework cocoa \
             -framework CoreAudio \
             -framework AudioToolbox \
             -framework CoreFoundation

COMPILER_OPTIONS = -g -I$(CLAP_SDK_PATH)/include -fPIC
CC = clang

# Define both library targets
SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so
STATIC_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.a

.PHONY: all clean ylc_bindings static shared

# Build both static and shared libraries by default
all: $(SHARED_LIB_TARGET) $(STATIC_LIB_TARGET)

# Define targets for building specific library types
shared: $(SHARED_LIB_TARGET)
static: $(STATIC_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/faust

# Build engine object files - compile with position independent code for compatibility
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -fPIC -c -o $@ $<

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
	@echo "Creating shared library $(SHARED_LIB_TARGET)"
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
	install_name_tool -id @rpath/libyalce_synth.so $@

# Build the static library
$(STATIC_LIB_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	# @echo "Creating static library $(STATIC_LIB_TARGET)"
	# @rm -f $@
	ar rcs $@ $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	# ranlib $@

# Run bindings generation after building either library
libs: $(SHARED_LIB_TARGET) $(STATIC_LIB_TARGET)
	$(MAKE) -C ../tools
	$(MAKE) -C . ylc_bindings

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
