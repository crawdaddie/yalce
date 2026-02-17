include ../../.env

CC     = clang
LLVM   = ${LLVM_PATH}
LLVM_CONFIG = $(LLVM)/bin/llvm-config

CFLAGS  = -Wall -Wextra -fPIC -g
CFLAGS += -I../../lang
CFLAGS += -I../../lang/backend_llvm
CFLAGS += `$(LLVM_CONFIG) --cflags`
CFLAGS += -I`$(LLVM_CONFIG) --includedir`

LIB_NAME    = libaudio_jit
DYNAMIC_LIB = $(LIB_NAME).so

SOURCES = audio_jit.c
OBJECTS = $(SOURCES:.c=.o)

all: $(DYNAMIC_LIB)

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(DYNAMIC_LIB)

.PHONY: all clean
