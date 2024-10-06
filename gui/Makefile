# gui/Makefile

CC := clang
CFLAGS := -Wall -Wextra -I.
LDFLAGS := -lSDL2

BUILD_DIR := ../build/gui

SRCS := gui.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(BUILD_DIR)/libgui.a

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c gui.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libgui.a: $(OBJS)
	ar rcs $@ $(OBJS)

clean:
	rm -rf $(BUILD_DIR)
