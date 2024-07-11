#  yalce_synth
simple C synthesis environment  
highly unsafe

## dependencies
- [libsoundio](http://libsound.io/)
- [raylib](https://www.raylib.com/)
- [fftw3](https://www.fftw.org/)
- [llvm](https://llvm.org/)

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

the flow of a signal between nodes A and B is represented by node B containing an input buffer reference that points to the output of node A
therefore node A must appear earlier in the graph than node B.

This constraint should be enforced by consumers of the library by embedding nodes in the graph only after their dependencies have been embedded.


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
REPL - want to be able to iteratively modify the running audio graph  

## features / syntax
the DSL is a language loosely based on ocaml syntax for creating and linking audio node objects

it supports declaring external C functions:
```ocaml
let init_audio  = extern fn () -> () ;; # audio engine library init func
let printf      = extern fn string -> () ;; # simplified printf from c stdlib

let () = init_audio ();
let () = printf "hello\n"
```
basic arithmetic, functions, currying and piping:
```ocaml
(1 + 2) * 8;

let f = fn x y z -> x + y + z ;;

let g = f 1 2;

g 3 # returns: 6

3 |> f 1 2; # evaluates f 1 2 3 
```
recursion and basic pattern-matching:
```ocaml
let fib = fn x ->
  (match x with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (x - 1)) + (fib (x - 2))
  )
;;
```
destructuring values in let bindings, function parameters and more complex pattern-matching:
```ocaml
let (x, _) = (1, 2) in x + 1;
let first = fn (a, _) -> a ;;

let complex_match = fn x -> 
  (match x with
  | (1, _) -> 0
  | (2, z) -> 100 + z
  | _      -> 1000
  )
;;

complex_match (2, 2);

let list_sum = fn acc l ->
  (match l with
  | [] -> acc
  | head::rest -> list_sum (acc + head) rest
  )
;;

list_sum 0 [1, 2, 3] # --returns 6 
```

and type inference:

this function
```ocaml
let first = fn (a, _) ->
    a
;;
```
is typed as `((t1 * t2) ->  t1)` - in other words it maps a tuple `'t1 * 't2` to `'t1`

since this function is generic in t1 & t2 (even though t2 isn't used)
it's compilation in the LLVM backend is deferred until it's called with concrete parameters, 

eg `first (1, "hi")` results in the compilation and caching of a concrete version of 
`first`

which looks like this in LLVM IR:
```
define i32 @"first[(Int * String)]"({ i32, ptr } %0) {
entry:
  %struct_element = extractvalue { i32, ptr } %0, 0
  ret { i32, ptr } %0
}
```

monomorphization and type inference is necessary when using the LLVM backend because boxed or tagged datatypes aren't implemented and all function inputs and sizes must be known when compiling




 
## Build lang executable
```
make
```
builds the build/audio_lang executable using a tree-walk interpreter as the evaluator

```
make LLVM_BACKEND=1
```
builds the build/audio_lang executable using the LLVM JIT-compiler as a backend backend 

## Usage
```
build/audio_lang -i
```
run the lang interpreter / compiler as a repl

```
build/audio_lang filename.ylc
```
compile and run the file filename.ylc

```
build/audio_lang filename.ylc -i
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
