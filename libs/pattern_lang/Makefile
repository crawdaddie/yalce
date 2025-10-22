CC = clang
CFLAGS := -g -O3 -fPIC -std=c99 -Wall -Wextra
# CFLAGS := -g -fPIC -std=c99 
CFLAGS += -I../../lang

LDFLAGS = -shared



TARGET = libpatternlang.so
SOURCES = pattern_lang.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

