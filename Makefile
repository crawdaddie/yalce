src = src/main.c src/user_ctx.c src/audio/graph.c src/queue.c src/oscilloscope.c src/scheduling.c src/buf_read.c

obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile

synth: $(obj)
	gcc -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth
