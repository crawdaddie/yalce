src = src/main.c 
# src += src/graph/graph.c
src += src/scheduling.c 
src += src/lang/lexer.c
src += src/lang/lang.c
src += src/lang/dbg.c

obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth



src_lang = $(wildcard src/lang/*.c)

obj_lang = $(src_lang:.c=.o)
lang: $(obj_lang)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)


.PHONY: test_lang
test_lang:
	make clean
	rm -f src/lang/parse.c src/lang/parse.h src/lang/lex.c src/lang/lex.h;
	flex -o src/lang/lex.c src/lang/lex.l
	bison -dy -b src/lang/parse src/lang/parse.y
	make lang

TEST_DIR = src/lang/test

.PHONY: test_suite
test_suite: $(TEST_DIR)/*
	for file in $^ ; do \
		./test_file.sh $${file} ; \
  done
	
