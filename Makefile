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
src += src/midi.c
src += src/msg_queue.c

# src += $(wildcard src/graph/*.c)
# src += $(wildcard src/lang/*.c)
# src += src/bindings.c

obj = $(src:.c=.o)

CC = clang 


LDFLAGS = -lsoundio -lm -lsndfile
FRAMEWORKS =-framework opengl -framework cocoa -framework CoreMIDI -framework CoreFoundation
COMPILER_OPTIONS = -Werror -Wall -Wextra

synth: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(COMPILER_OPTIONS)

EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC 
libyalce_synth.so: $(obj)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FRAMEWORKS) $(EXPORT_COMPILER_OPTIONS)

ocamlobj = $(wildcard ocaml/*.cmo)

.PHONY: ocaml_make
ocaml_make:
	ocamlfind ocamlc -I ocaml/ -package lwt.unix -i ocaml/$(name).ml > ocaml/$(name).mli
	ocamlfind ocamlc -I ocaml/ -package lwt.unix -c ocaml/$(name).mli -o ocaml/$(name).cmi
	ocamlfind ocamlc -I ocaml/ -package lwt.unix -c ocaml/$(name).ml -cmi-file ocaml/$(name).cmi

.PHONY: ocamlbindings
ocamlbindings:
	make libyalce_synth.so
	cp libyalce_synth.so ocaml/
	make name=stubs ocaml_make
	ocamlc -I ./ -c ocaml/stubs.c -cclib -L. -I src -cclib -lyalce_synth -o ocaml/stubs.o -cc $(CC)
	ocamlmklib -o ocaml/stubs -L. -lyalce_synth -L. ocaml/stubs.o

	make name=nodes ocaml_make
	make name=osc ocaml_make
	make name=fx ocaml_make
	make name=synths ocaml_make
	make name=seqq ocaml_make
	make name=midi ocaml_make

	ocamlc -a -custom -o ocaml/stubs.cma -dllib ocaml/dllstubs.so

.PHONY: utop_test
utop_test:
	./scripts/post_window.sh
	utop -I ./ocaml \
		-require unix \
		-require core \
		-require lwt.unix \
		ocaml/stubs.cma \
		ocaml/nodes.cmo \
		ocaml/osc.cmo \
		ocaml/fx.cmo \
		ocaml/synths.cmo \
		ocaml/seqq.cmo \
		ocaml/midi.cmo \
		-init examples/utop_init.ml

.PHONY: py
py:
	./scripts/post_window.sh
	python -i -c "import yalce_synth_py.bindings as audio"
.PHONY: cean
clean:
	rm -f $(obj) main

.PHONY: run
run:
	make clean && make synth && ./synth

