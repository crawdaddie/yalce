LEX_FILE = src/lex.l
YACC_FILE = src/parser.y
LEX_OUTPUT = src/lex.yy.c
YACC_OUTPUT = src/y.tab.c src/y.tab.h


SRCDIR := src
BUILDDIR := build
TESTDIR := test
CC = clang -I../include -I./src -g
LDFLAGS = -Lbuild -lyalce_synth -lm


SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o) $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR) $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(CC) $(CFLAGS) -c -o $@ $<

$(YACC_OUTPUT): $(YACC_FILE)
	bison -yd $(YACC_FILE) -b src/y

$(LEX_OUTPUT): $(LEX_FILE)
	flex -o $(LEX_OUTPUT) $(LEX_FILE)

.PHONY: all clean

all: build/audio_lang 

build/audio_lang: $(OBJS)
	cp ../build/libyalce_synth.so build/libyalce_synth.so
	$(CC) -o $@ $^ $(LDFLAGS) 


$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -Rf $(BUILDDIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT) parser

test_parse: $(TESTDIR)/test_parse.o $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o $(filter-out $(BUILDDIR)/main.o $(BUILDDIR)/test_eval.o, $(OBJS))
	$(CC) -o $(BUILDDIR)/$@ $^ $(LDFLAGS)
	./$(BUILDDIR)/test_parse

$(TESTDIR)/test_parse.o: $(TESTDIR)/test_parse.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)  

test_eval: $(TESTDIR)/test_eval.o $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o $(filter-out $(BUILDDIR)/main.o $(BUILDDIR)/test_parse.o, $(OBJS))
	$(CC) -o $(BUILDDIR)/$@ $^ $(LDFLAGS)
	./$(BUILDDIR)/test_eval

$(TESTDIR)/test_eval.o: $(TESTDIR)/test_eval.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)  


