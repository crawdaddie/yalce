src = src/main.c src/user_ctx.c src/audio/graph.c

obj = $(src:.c=.o)

LDFLAGS = -ljack -lm -lm

main: $(obj)
	gcc -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) main
