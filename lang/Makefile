LEX_FILE = src/lex.l
YACC_FILE = src/parser.y
LEX_OUTPUT = src/lex.yy.c
YACC_OUTPUT = src/y.tab.c src/y.tab.h


SRCDIR := src
BUILDDIR := build
TESTDIR := test
CC = clang -I../include -I./src -g

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
	$(CC) -o $@ $^


$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -Rf $(BUILDDIR)
	rm -f $(LEX_OUTPUT) $(YACC_OUTPUT) parser

test_parse: $(TESTDIR)/test_parse.o $(BUILDDIR)/y.tab.o $(BUILDDIR)/lex.yy.o $(filter-out $(BUILDDIR)/main.o, $(OBJS))
	$(CC) -o $(BUILDDIR)/$@ $^
	./$(BUILDDIR)/test_parse

$(TESTDIR)/test_parse.o: $(TESTDIR)/test_parse.c $(YACC_OUTPUT) $(LEX_OUTPUT)
	$(CC) $(CFLAGS) -c -o $@ $< 

