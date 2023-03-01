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

src_lang_test = $(src_lang)
src_lang_test += src/lang_test.c
obj_lang_test = $(src_lang_test:.c=.o)

lang: $(obj_lang_test)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

src = src/main.c 
src += src/scheduling.c 
src += $(src_lang)
obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	gcc -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: test_lang
test_lang:
	make clean
	make lang

TEST_DIR = src/lang/test

.PHONY: lang_test_suite
lang_test_suite: $(wildcard $(TEST_DIR)/*.test.simple)
	make test_lang
	for file in $^ ; do \
		./test_file.sh $${file} ; \
  done
	
