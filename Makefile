INCLUDE_DIR = include
src = src/start_audio.c
src += src/ctx.c
src += src/channel.c
src += src/scheduling.c
src += src/memory.c
src += src/node.c
src += src/log.c
src += src/dbg.c
src += src/write_sample.c
src += $(wildcard src/audio/*.c)
src += src/soundfile.c
src += src/msg_queue.c
src += src/midi.c

# src += src/oscilloscope.c
# src = src/main.c
# src += src/oscilloscope.c
# src += lib/tigr/tigr.c
# src += $(wildcard src/graph/*.c)
# src += $(wildcard src/lang/*.c)
# src += src/bindings.c

obj = $(src:.c=.o)

CC = cc -Iinclude


LDFLAGS = -lsoundio -lm -lsndfile
FRAMEWORKS =-framework opengl -framework CoreMIDI -framework cocoa
COMPILER_OPTIONS = -Werror -Wall -Wextra -Iinclude

synth: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)

EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -Iinclude

libyalce_synth.a: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

libyalce_synth.so: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS) -fPIC

# .PHONY install

install:
	mkdir -p /usr/local/include/yalce 
	cp -r $(INCLUDE_DIR)/* /usr/local/include/yalce
	rm /usr/local/lib/libyalce_synth.so
	cp libyalce_synth.so /usr/local/lib/libyalce_synth.so
	mkdir -p /usr/local/lib/pkgconfig
	cp libyalce_synth.pc /usr/local/lib/pkgconfig/libyalce_synth.pc
