INCLUDE_DIR = include
CC = clang -Iinclude

SRCDIR := src
BUILDDIR := build
SHARED_LIB_TARGET := $(BUILDDIR)/libyalce_synth.so

SRCS := $(wildcard $(SRCDIR)/*.c)

OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

LDFLAGS = -lsoundio -lm -lsndfile -lraylib -Llib/rubberband/build/ -lrubberband
FRAMEWORKS =-framework opengl -framework CoreMIDI -framework cocoa
# RAYLIB_INCLUDE=/opt/homebrew/include
# RAYLIB_LIB=/opt/homebrew/lib/
COMPILER_OPTIONS = -Werror -Wall -Wextra -Iinclude -g

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/audio

all: $(TARGET)
.PHONY: all clean install

clean:
	$(RM) -r $(BUILDDIR)

build/synth: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)


EXPORT_COMPILER_OPTIONS = $(COMPILER_OPTIONS) -fPIC

$(SHARED_LIB_TARGET): $(filter-out $(BUILDDIR)/main.o, $(OBJS))
	$(CC) -shared -o $@  $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

install:
	mkdir -p /usr/local/include/yalce 
	cp -r $(INCLUDE_DIR)/* /usr/local/include/yalce
	rm /usr/local/lib/libyalce_synth.so
	cp build/libyalce_synth.so /usr/local/lib/libyalce_synth.so
	mkdir -p /usr/local/lib/pkgconfig
	cp libyalce_synth.pc /usr/local/lib/pkgconfig/libyalce_synth.pc
	cp lib/rubberband/lib/librubberband.a /usr/local/lib/librubberband.a
	cp lib/rubberband/rubberband.pc.in /usr/local/lib/pkgconfig/librubberband.pc
#
# libyalce_synth.a: $(obj)
# 	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)
#
# libyalce_synth.so: $(obj)
# 	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS) -fPIC
#
# # .PHONY install
#
