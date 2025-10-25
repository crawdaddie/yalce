**YALCE / YLC** (Yet Another Live-Coding Environment) is a functional programming language bundled with libraries for audio synthesis and OpenGL graphics.  
YLC aims to be small, simple and performant, with a minimal set of features and seamless C interoperability

## Key Features

- **Functional Programming** - OCaml-like syntax with first-class functions
- **Type Inference** - Hindley-Milner type system, with some typeclass-based extensions 
- **LLVM JIT** - Compiles to native targets using the LLVM JIT backend
- **Interactive REPL** - Iterative development and live coding
- **Pattern Matching** - Algebraic data types
- **Coroutines** - Generators and async iteration

## Quick Start

### Install via Homebrew (macOS)
```bash
brew install crawdaddie/yalce/yalce
```
### Or build from source
```bash
git clone https://github.com/crawdaddie/yalce
cd yalce
./setup.sh
make

# replace the installation path below with anywhere you prefer that's on your $PATH
ln -s "$PWD/build/ylc" "$HOME/.local/bin/ylc"
```

### Main Sections

- [Quick Start](getting_started.md) - Installation and first steps
- [Core Concepts](core_concepts.md) - main language features
- [Examples](examples.md) 
- [Reference](reference.md) - language reference 


## Examples

### Fibonacci with pattern matching
```ocaml
let fib = fn x ->
  match x with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (x - 1)) + (fib (x - 2))
;;
```

### First-class functions
```ocaml
let list_rev = fn l ->
  let aux = fn l res ->
    match l with
    | [] -> res
    | x :: rest -> aux rest (x :: res)
  ;;
  aux l []
;;

let list_map = fn f l ->
  let aux = fn f l res -> 
    match l with
    | [] -> res
    | x :: rest -> aux f rest (f x :: res) 
  ;;
  aux f l [] |> list_rev
;;

let test = module () ->
  let test_map_plus = (list_map ((+) 1) [0,1,2,3]) == [1,2,3,4];
;
```


