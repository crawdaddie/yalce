src = src/main.c
src += src/ctx.c
src += src/channel.c
src += src/scheduling.c
src += src/oscilloscope.c
src += src/memory.c
src += src/node.c
src += src/audio/sq.c
src += src/audio/math.c

# src += $(wildcard src/graph/*.c)
src += $(wildcard src/lang/*.c)
# src += $(wildcard src/audio/*.c)
src += src/bindings.c

obj = $(src:.c=.o)

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	clang -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth

src_lang = $(filter-out src/main.c, $(src))
src_lang += lang_test.c
obj_lang = $(src_lang:.c=.o)

lang: $(obj_lang)
	clang -o $@ $^ $(LDFLAGS) $(COMPILER_OPTIONS)

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
	

