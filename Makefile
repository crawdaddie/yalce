src = src/main.c 
# src += src/graph/graph.c
src += src/scheduling.c 
src += src/lang/lexer.c
src += src/lang/parser.c

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

.PHONY: lang
lang:
	flex -o src/lang/lexer.c --header-file=src/lang/lexer.h src/lang/tokens.l
	bison -d src/lang/parser.y -o src/lang/parser.c

.PHONY: run
run:
	make clean && make lang && make synth && ./synth

.PHONY: test_lang
test_lang:
	make clean && make lang && gcc src/lang_test.c src/lang/lexer.c src/lang/parser.c -o lang && ./lang
