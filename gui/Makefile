# gui/Makefile

CC := clang -g
CFLAGS := -Wall -Wextra -I. -I$(shell brew --prefix sdl2)/include/SDL2 -I$(shell brew --prefix sdl2_ttf)/include/SDL2 -I$(shell brew --prefix sdl2_gfx)/include/SDL2


LDFLAGS := -L$(shell brew --prefix sdl2)/lib -lSDL2 -L$(shell brew --prefix sdl2_ttf)/lib -lSDL2_ttf -L$(shell brew --prefix sdl2_gfx)/lib -lSDL2_gfx
BUILD_DIR := ../build/gui

SRCS := gui.c edit_graph.c common.c slider_window.c
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
