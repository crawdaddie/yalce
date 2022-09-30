#test: test.c term.c
main: asimp.c audio/audio.c audio/util.c
	gcc -o main asimp.c audio/audio.c audio/util.c -lsoundio -lm -lpthread -I.
