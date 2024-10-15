# engine/Makefile

BUILD_DIR := ../build/engine
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

LDFLAGS = -lsoundio -lm -lsndfile -lfftw3
FRAMEWORKS = -framework opengl -framework CoreMIDI -framework cocoa -framework CoreAudio -framework AudioToolbox -framework CoreFoundation
COMPILER_OPTIONS = -g

CC = clang

SHARED_LIB_TARGET := $(BUILD_DIR)/libyalce_synth.so

.PHONY: all clean ylc_bindings

all: $(SHARED_LIB_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

ylc_bindings:
	$(MAKE) -C ../tools
	../build/tools/ffi_gen oscillators.h lib.h filters.h > bindings/Synth.ylc
	cat bindings/_engine_lib_extra.ylc >> bindings/Synth.ylc
	../build/tools/ffi_gen scheduling.h > bindings/Sched.ylc

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
