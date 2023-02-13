src = src/main.c 
# src += src/graph/graph.c
src += src/scheduling.c 
src += src/lang/lexer.c
src += src/lang/lang.c
src += src/lang/dbg.c

obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile -lcheck
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth



src_lang = src/lang/lang_test.c
src_lang += src/lang/lexer.c
src_lang += src/lang/lang.c
src_lang += src/lang/dbg.c
obj_lang = $(src_lang:.c=.o)
lang: $(obj_lang)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: test_lang
test_lang:
	make clean
	make lang
	./lang lang_test.txt
