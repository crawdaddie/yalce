include ../.env
# gui/Makefile

# VST Library path
VST_LIB_PATH := ${VST_LIB_PATH}
VST_BUILD_PATH := $(VST_LIB_PATH)/build
CC := clang -g
CFLAGS := -Wall -Wextra -fPIC -I. \
					-I${SDL2_PATH}/include \
					-I${SDL2_PATH}/include/SDL2 \
					-I${SDL2_TTF_PATH}/include \
					-I${SDL2_GFX_PATH}/include \
					-I$(VST_LIB_PATH)/src

LDFLAGS := -L$(shell brew --prefix sdl2)/lib -lSDL2 \
					 -L$(shell brew --prefix sdl2_ttf)/lib -lSDL2_ttf \
					 -L$(shell brew --prefix sdl2_gfx)/lib -lSDL2_gfx \
					 -L$(shell brew --prefix sdl2_image)/lib -lSDL2_image \
					 -L$(VST_BUILD_PATH) -lylcvst \
					 -lGLEW -lglfw

FRAMEWORKS := -framework Foundation -framework OpenGL 

BUILD_DIR := ../build/gui
# Add Objective-C file for macOS VST support
# OBJC_SRCS := CocoaVSTHelper.m
# OBJC_OBJS := $(OBJC_SRCS:%.m=$(BUILD_DIR)/%.o)

SRCS := gui.c gl.c array_edit.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

# all: $(BUILD_DIR)/libgui.a

all: $(BUILD_DIR)/libgui.so

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c gui.h gl.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Special rule for Objective-C files
# $(BUILD_DIR)/%.o: %.m | $(BUILD_DIR)
# 	$(CC) $(CFLAGS) -ObjC -c $< -o $@ $(FRAMEWORKS)

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
	../build/tools/ffi_gen array_edit.h >> bindings/Gui.ylc

	cat bindings/_gui_extra.ylc >> bindings/Gui.ylc
