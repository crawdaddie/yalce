#  yalce_synth
simple C audio synthesis library & high-level language
highly unsafe

## dependencies
- [libsoundio](http://libsound.io/)
- [raylib](https://www.raylib.com/)
- [fftw3](https://www.fftw.org/)
- [llvm](https://llvm.org/)
- [readline](https://tiswww.case.edu/php/chet/readline/rltop.html)

# Audio Engine:
## Architecture
Sets up a loop in a realtime thread (handled by libsoundio) that traverses a
graph of audio nodes, computes their summed outputs and writes the result to the audio card

the graph can be updated directly with new nodes but updates can be handled in
a more orderly way that allows for precisely synchronized updates by writing
messages to a ring buffer shared by the RT thread (read-only) and the main
thread (write-only)

an audio node is an object that holds 
- a pointer to some internal state (eg oscillator phase)
- an output buffer
- references to input buffers
- a pointer to a `node_perform` function that reads from the input buffers, and writes to the output buffers
- a reference to the next node in the chain to compute

the flow of a signal between nodes A and B is represented by
node B containing an input buffer reference that points to the output of node A
therefore node A must appear earlier in the graph than node B.


## build
```
make audio_test
```
builds & runs a standalone audio engine [test program](engine/main.c) that plays a square wave

```
make build/libyalce_synth.so
```
builds the audio engine as a shared-object library


# Lang
## Requirements in order of importance
- [ ] REPL - want to be able to iteratively modify the running audio graph  
- [ ] easy C interop  

## features / syntax
the DSL is a language loosely based on ocaml syntax for creating and linking audio node objects

## Build lang executable
```
make
```
builds the executable `lang` in build/ using the LLVM JIT-compiler as a backend

### build flags:
`GUI_MODE=1` -- builds the executable with gui enabled 
`DUMP_AST=1` -- displays the program's AST

## Usage
```
build/lang -i
```
run the lang interpreter / JIT compiler as a repl

```
build/lang filename.ylc
```
compile and run the file filename.ylc

```
build/lang filename.ylc -i
```
compile and run the file filename.ylc and continue to accept interactive repl input



## Audio lang tests
```
make test_parse
```
test the parser

```
make test_typecheck
```
test the lang's type inference
