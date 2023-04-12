src = src/main.c
src += src/ctx.c
src += src/channel.c
src += src/scheduling.c
src += src/oscilloscope.c
src += src/memory.c
src += src/node.c
src += src/log.c
src += src/audio/osc.c
src += src/audio/delay.c
# src += src/audio/blip.c
src += src/audio/math.c
src += src/start_audio.c
src += src/write_sample.c
# src += src/callback.c
src += src/audio/signal.c
src += src/oscilloscope.c
src += src/audio/out.c
src += lib/tigr.c

# src += $(wildcard src/graph/*.c)
# src += $(wildcard src/lang/*.c)
# src += $(wildcard src/audio/*.c)
# src += src/bindings.c

obj = $(src:.c=.o)

CC = clang 

LDFLAGS = -lsoundio -lm -lSDL2 -lsndfile

FRAMEWORKS =-framework opengl -framework cocoa
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)


EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC

libsimpleaudio.so: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

# .PHONY: ocamlbindings
# ocamlbindings:
# 	make libsimpleaudio.so
# 	ocamlc -i simpleaudio_stubs.ml > simpleaudio_stubs.mli
# 	ocamlc -c simpleaudio_stubs.mli
# 	ocamlc -c simpleaudio_stubs.ml
# 	ocamlc -I ./ -c simpleaudio_stubs.c -cclib -L. -I src -cclib -lsimpleaudio -o simpleaudio_stubs.o
# 	ocamlmklib -o simpleaudio_stubs -L. -lsimpleaudio -L. simpleaudio_stubs.o
# 	ocamlc -a -custom -o simpleaudio_stubs.cma simpleaudio_stubs.cmo -dllib dllsimpleaudio_stubs.so

.PHONY: ocamlbindings
ocamlbindings:
	make libsimpleaudio.so
	cp libsimpleaudio.so ocaml/
	ocamlc -i ocaml/simpleaudio_stubs.ml > ocaml/simpleaudio_stubs.mli
	ocamlc -c ocaml/simpleaudio_stubs.mli -o ocaml/simpleaudio_stubs.cmi
	ocamlc -c ocaml/simpleaudio_stubs.ml -cmi-file ocaml/simpleaudio_stubs.cmi
	ocamlc -I ./ -c ocaml/simpleaudio_stubs.c -cclib -L. -I src -cclib -lsimpleaudio -o ocaml/simpleaudio_stubs.o
	ocamlmklib -o ocaml/simpleaudio_stubs -L. -lsimpleaudio -L. ocaml/simpleaudio_stubs.o
	ocamlc -a -custom -o ocaml/simpleaudio_stubs.cma ocaml/simpleaudio_stubs.cmo -dllib ocaml/dllsimpleaudio_stubs.so

.PHONY: utop_test
utop_test:
	echo "" > mylog.txt
	./scripts/post_window.sh
	utop -I ./ocaml -require unix -require core -require lwt ocaml/simpleaudio_stubs.cma -init examples/utop_init.ml

.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth

