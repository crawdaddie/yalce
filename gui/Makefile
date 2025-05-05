include ../.env
# gui/Makefile
CLAP_SDK_PATH := ${CLAP_PATH}
CC := clang -g
CFLAGS := -Wall -Wextra -fPIC -I. \
					-I${SDL2_PATH}/include \
					-I${SDL2_PATH}/include/SDL2 \
					-I${SDL2_TTF_PATH}/include \
					-I${SDL2_GFX_PATH}/include \
					-I$(CLAP_SDK_PATH)/include

LDFLAGS := -L$(shell brew --prefix sdl2)/lib -lSDL2 \
					 -L$(shell brew --prefix sdl2_ttf)/lib -lSDL2_ttf \
					 -L$(shell brew --prefix sdl2_gfx)/lib -lSDL2_gfx

BUILD_DIR := ../build/gui

SRCS := gui.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

# all: $(BUILD_DIR)/libgui.a

all: $(BUILD_DIR)/libgui.so

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c gui.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libgui.a: $(OBJS)
	ar rcs $@ $(OBJS)
	$(MAKE) -C . ylc_bindings

# Build the shared library
$(BUILD_DIR)/libgui.so: $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
	install_name_tool -id @rpath/libgui.so $@
	$(MAKE) -C . ylc_bindings

# $(BUILD_DIR)/libgui.so: $(OBJS)
# 	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

ylc_bindings:
	$(MAKE) -C ../tools
	../build/tools/ffi_gen gui.h > bindings/Gui.ylc

	cat bindings/_gui_extra.ylc >> bindings/Gui.ylc
