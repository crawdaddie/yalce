src = src/main.c
src += src/start_audio.c
src += src/ctx.c
src += src/channel.c
src += src/scheduling.c
src += src/oscilloscope.c
src += src/memory.c
src += src/node.c
src += src/log.c
src += src/dbg.c
src += src/write_sample.c
src += src/oscilloscope.c
src += lib/tigr/tigr.c
src += lib/fe/src/fe.c
src += $(wildcard src/audio/*.c)
src += src/soundfile.c
src += src/msg_queue.c
src += src/lang/dsl.c

# src += $(wildcard src/graph/*.c)
# src += $(wildcard src/lang/*.c)
# src += src/bindings.c

obj = $(src:.c=.o)

CC = clang

LDFLAGS = -lsoundio -lm -lsndfile -ldl
FRAMEWORKS =-framework opengl -framework cocoa 
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)

EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC 
libyalce_synth.so: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth


test_src = $(filter-out src/main.c, $(wildcard src/*.c))
test_src += $(wildcard src/audio/*.c)

%: tests/%.c
	@$(CC) $(LDFLAGS) $(FRAMEWORKS) \
		-DUNITY_SUPPORT_64 \
		-DUNITY_OUTPUT_COLOR \
		lib/Unity/src/unity.c \
		lib/tigr/tigr.c \
		-L. -I src -lyalce_synth \
		$^ -o $@

.PHONY: test
test:
	make libyalce_synth.so
	make node.test && ./node.test
	make msg_queue.test && ./msg_queue.test

