include ../.env
# engine/Makefile

BUILD_DIR := ../build/engine_blobs
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
LDFLAGS = -lsoundio -lm -lsndfile -lfftw3 -ldl -lxml2
FRAMEWORKS = -framework opengl \
						 -framework CoreMIDI \
						 -framework cocoa \
						 -framework CoreAudio \
						 -framework AudioToolbox \
						 -framework CoreFoundation

COMPILER_OPTIONS = -g 
CC = clang

.PHONY: clean


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build engine object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(COMPILER_OPTIONS) -c -o $@ $<

audio_test: $(OBJS)
	$(CC) $(LDFLAGS) $(FRAMEWORKS) $(OBJS) -o $(BUILD_DIR)/engine_test
	$(BUILD_DIR)/engine_test

clean:
	rm -rf $(BUILD_DIR)
