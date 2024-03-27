INCLUDE_DIR = include
CC = clang -Iinclude -g

SRCDIR := src
BUILDDIR := build
SHARED_LIB_TARGET := $(BUILDDIR)/libyalce_synth.so

SRCS := $(wildcard $(SRCDIR)/*.c)

OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

LDFLAGS = -lsoundio -lm -lsndfile -lraylib -lfftw3
# -Llib/rubberband/build/ -lrubberband
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
.PHONY: all clean

synth: build/synth
	./build/synth

clean:
	rm -r $(BUILDDIR)
	dune clean 

build/synth: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)


EXPORT_COMPILER_OPTIONS = $(COMPILER_OPTIONS) -fPIC

$(SHARED_LIB_TARGET): $(filter-out $(BUILDDIR)/main.o, $(OBJS))
	$(CC) -shared -o $@  $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

OCAML_EXAMPLE_DIR := examples

$(OCAML_EXAMPLE_DIR)/%.ml: $(SHARED_LIB_TARGET) .touch_file
	dune exec $(basename $@).exe --profile release

OCAML_EXAMPLE_BUILD_DIR := _build/default/examples

$(OCAML_EXAMPLE_BUILD_DIR)/%.exe: $(SHARED_LIB_TARGET)
	# $(OCAML_EXAMPLE_DIR)/$*.ml
	# echo $(notdir $@) 
	dune build $(OCAML_EXAMPLE_DIR)/$(notdir $@) --profile release
	./$@



.touch_file: $(SHARED_LIB_TARGET)
	touch .touch_file

.PHONY: examples utop
examples: $(OCAML_EXAMPLE_DIR)/%.ml

utop:
	dune utop
