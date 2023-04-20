src = src/main.c
src += src/start_audio.c
src += src/ctx.c
src += src/channel.c
src += src/scheduling.c
src += src/oscilloscope.c
src += src/memory.c
src += src/node.c
src += src/log.c
src += src/dbg.c
src += src/write_sample.c
src += src/oscilloscope.c
src += lib/tigr.c
src += $(wildcard src/audio/*.c)
src += src/soundfile.c

# src += $(wildcard src/graph/*.c)
# src += $(wildcard src/lang/*.c)
# src += src/bindings.c

obj = $(src:.c=.o)

CC = clang 

LDFLAGS = -lsoundio -lm -lsndfile

FRAMEWORKS =-framework opengl -framework cocoa
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)


EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC

libsimpleaudio.so: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

ocamlobj = $(wildcard ocaml/*.cmo)

.PHONY: ocaml_make
ocaml_make:
	ocamlc -I ocaml/ -i ocaml/$(name).ml > ocaml/$(name).mli
	ocamlc -I ocaml/ -c ocaml/$(name).mli -o ocaml/$(name).cmi
	ocamlc -I ocaml/ -c ocaml/$(name).ml -cmi-file ocaml/$(name).cmi

.PHONY: ocamlbindings
ocamlbindings:
	make libsimpleaudio.so
	cp libsimpleaudio.so ocaml/
	make name=stubs ocaml_make
	ocamlc -I ./ -c ocaml/stubs.c -cclib -L. -I src -cclib -lsimpleaudio -o ocaml/stubs.o -cc $(CC)
	ocamlmklib -o ocaml/stubs -L. -lsimpleaudio -L. ocaml/stubs.o

	make name=nodes ocaml_make
	make name=osc ocaml_make
	make name=fx ocaml_make
	make name=synths ocaml_make

	ocamlfind ocamlc -package lwt.unix -i ocaml/seqq.ml > ocaml/seqq.mli
	ocamlfind ocamlc -package lwt.unix -c ocaml/seqq.mli -o ocaml/seqq.cmi
	ocamlfind ocamlc -package lwt.unix -c -cmi-file ocaml/seqq.cmi ocaml/seqq.ml

	ocamlc -a -custom -o ocaml/stubs.cma -dllib ocaml/dllstubs.so

.PHONY: utop_test
utop_test:
	./scripts/post_window.sh
	echo $(ocamlobj)
	utop -I ./ocaml \
		-require unix \
		-require core \
		-require lwt.unix \
		ocaml/stubs.cma ocaml/nodes.cmo ocaml/osc.cmo ocaml/fx.cmo ocaml/synths.cmo ocaml/seqq.cmo \
		-init examples/utop_init.ml

.PHONY: py
py:
	./scripts/post_window.sh
	python -i -c "import simpleaudio_py.bindings as audio"
.PHONY: clean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth

