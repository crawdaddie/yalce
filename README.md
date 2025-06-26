#  yalce_synth
simple C audio synthesis library & high-level language
highly unsafe

## dependencies
- [llvm](https://llvm.org/)
- [readline](https://tiswww.case.edu/php/chet/readline/rltop.html)
- [libsoundio](http://libsound.io/)
- [fftw3](https://www.fftw.org/)
- [sdl2](https://www.libsdl.org/)

# Audio Engine:
## Architecture
Sets up a loop in a realtime thread (handled by libsoundio) that traverses a
graph of audio nodes, computes their summed outputs and writes the result to the audio card

new nodes are created outside the loop, but the graph can be updated precisely by writing
messages to a ring buffer shared by the RT thread audio-loop thread (read-only) and the main
thread (write-only)

an audio node is an object that holds 
- a pointer to some internal state (eg oscillator phase)
- an output buffer
- a `node_perform` function pointer that reads from the input buffers, and writes to the output buffers
- a reference to the next node in the chain to compute

## install
```
brew install crawdaddie/yalce/yalce
```
## build
```
make build/libyalce_synth.so
```
builds the audio engine as a shared-object library


# Lang
[full language reference](./docs/ylc-ref.md)

## Requirements in order of importance
- [x] REPL - want to be able to iteratively modify the running audio graph  
- [x] easy C interop  


## features / syntax
the language has syntax superficially similar to ocaml for creating and linking audio node objects

```ocaml
# fns
let f = fn a b ->
    a + b
;;
```

```ocaml
#recursive fns and match expressions
let fib = fn x ->
  match x with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (x - 1)) + (fib (x - 2))
;;
```

## Build lang executable
```
make
```
builds the executable `ylc` in build/ using the LLVM JIT-compiler as a backend

## Usage
```
ylc -i
```
run the lang JIT compiler as a repl

```
ylc filename.ylc
```
compile and run the file filename.ylc

```
ylc filename.ylc -i
```
compile and run the file filename.ylc and continue to accept interactive repl input

```
ylc --test filename.ylc
```
compile and run the tests from the module filename.ylc



## Audio lang tests
```
make test_parse
```
test the parser

```
make test_typecheck
```
test the lang's type inference

```
make test_scripts
```
run a set of .ylc test scripts 
