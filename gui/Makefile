include ../.env
# gui/Makefile

# VST Library path
# VST_LIB_PATH := ${VST_LIB_PATH}
# VST_BUILD_PATH := $(VST_LIB_PATH)/build
CC := clang -g
CC += -DYLC_LANG_DYN_LIB
CFLAGS := -Wall -Wextra -fPIC -I. \
					-I${SDL2_PATH}/include \
					-I${SDL2_PATH}/include/SDL2 \
					-I${SDL2_TTF_PATH}/include \
					-I${SDL2_GFX_PATH}/include \
					-I../lang
					# -I$(VST_LIB_PATH)/src \

ifeq ($(shell uname -s),Darwin)
    LDFLAGS := -L$(shell brew --prefix sdl2)/lib -lSDL2 \
               -L$(shell brew --prefix sdl2_ttf)/lib -lSDL2_ttf \
               -L$(shell brew --prefix sdl2_gfx)/lib -lSDL2_gfx \
               -L$(shell brew --prefix sdl2_image)/lib -lSDL2_image \
               -lGLEW -lglfw
    FRAMEWORKS := -framework Foundation -framework OpenGL
else
    LDFLAGS := -lSDL2 -lSDL2_ttf -lSDL2_gfx -lSDL2_image -lGLEW -lglfw -lGL
    FRAMEWORKS :=
endif 

BUILD_DIR := build/

SRCS := gui.c gl.c plt.c array_edit.c decl_ui.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(BUILD_DIR)/libgui.so

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c gui.h gl.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/libgui.a: $(OBJS)
	ar rcs $@ $(OBJS)
	$(MAKE) -C . ylc_bindings

$(BUILD_DIR)/libgui.so: $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS)
ifeq ($(shell uname -s),Darwin)
	install_name_tool -id @rpath/libgui.so $@
endif
	$(MAKE) -C . ylc_bindings

clean:
	rm -rf $(BUILD_DIR)

ylc_bindings:
	# $(MAKE) -C ../tools
	# @echo "let _ = dlopen \"../build/libgui.so\";" > bindings/Gui.ylc
	# ../build/tools/ffi_gen gui.h >> bindings/Gui.ylc
	# ../build/tools/ffi_gen array_edit.h >> bindings/Gui.ylc
	# ../build/tools/ffi_gen plt.h >> bindings/Gui.ylc
	# ../build/tools/ffi_gen decl_ui.h >> bindings/Gui.ylc
	# cat bindings/_gui_extra.ylc >> bindings/Gui.ylc
