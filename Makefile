src = src/main.c 
# src += src/graph/graph.c
src += src/scheduling.c 
src += src/lang/lexer.c
src += src/lang/parser.c
src += src/lang/lang.c

obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile -lcheck
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	rm src/lang/*.c
	rm src/lang/*.h
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make lang && make synth && ./synth

.PHONY: test_lang
test_lang:
	make clean
	gcc src/lang/lang_test.c src/lang/lang.c -o lang
	./lang lang_test.txt
