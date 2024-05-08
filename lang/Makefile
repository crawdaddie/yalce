SRCDIR := src
BUILDDIR := build

SRCS := $(wildcard $(SRCDIR)/*.c)

OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Variables
# INCLUDE_DIR = ../include
CC = clang -I../include -Iinclude -g
# CFLAGS = -Werror -Wall -Wextra
#
# LDFLAGS =

# Targets
all: build/audio_lang

build/audio_lang: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)

# main.o: src/main.c
# 	$(CC) $(CFLAGS) -c src/main.c

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

.PHONY: clean run
clean:
	rm -Rf $(BUILDDIR)

run:
	$(MAKE) all
	./build/audio_lang
