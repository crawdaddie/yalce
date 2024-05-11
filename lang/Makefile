SRCDIR := src
BUILDDIR := build
TESTDIR := test

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

.PHONY: clean run test
clean:
	rm -Rf $(BUILDDIR)

run:
	$(MAKE) all
	./build/audio_lang -i

test_parse: $(TESTDIR)/test_parse.c
	$(CC) -o $(BUILDDIR)/$@ \
		$(filter-out $(BUILDDIR)/main.o, $(OBJS)) \
		$(TESTDIR)/$@.c $(LDFLAGS) \
		$(FRAMEWORKS) $(COMPILER_OPTIONS)

	./$(BUILDDIR)/test_parse

