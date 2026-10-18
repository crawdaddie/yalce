CC ?= cc
CFLAGS ?= -O2 -g
WARNFLAGS := -Wall -Wextra -Wpedantic
INCLUDES := -I../../lang

LIB_NAME := libfile_watch
OBJECT := file_watch.o
STATIC_LIB := $(LIB_NAME).a
SHARED_LIB := $(LIB_NAME).so

CFLAGS += $(WARNFLAGS) $(INCLUDES) -std=c11 -fPIC

.PHONY: all static clean

all: $(SHARED_LIB)

$(SHARED_LIB): $(OBJECT)
	$(CC) -shared -o $@ $^ -pthread

static: $(STATIC_LIB)

$(STATIC_LIB): $(OBJECT)
	ar rcs $@ $^

$(OBJECT): file_watch.c file_watch.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECT) $(STATIC_LIB) $(SHARED_LIB)
