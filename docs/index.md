# YALCE Documentation

**YALCE** (Yet Another Language & C Engine) is a functional programming language with LLVM JIT compilation designed specifically for real-time audio synthesis.

## Quick Start

```bash
# Install via Homebrew (macOS)
brew install crawdaddie/yalce/yalce

# Or build from source
make
```

## Documentation

- [Getting Started](getting-started.md) - Installation and first steps
- [Language Reference](ylc-ref.md) - Complete language syntax and features
- [Type System](type-system.md) - Hindley-Milner type inference
- [Audio Engine](audio-engine.md) - Real-time audio processing
- [Examples](examples.md) - Code examples and tutorials
- [API Reference](api-reference.md) - Standard library and built-ins

## Key Features

- **Functional Programming** - OCaml-like syntax with first-class functions
- **Type Inference** - Hindley-Milner type system with polymorphism
- **LLVM JIT** - Fast compilation with LLVM backend
- **Real-time Audio** - Lock-free audio graph processing
- **Interactive REPL** - Iterative development and live coding
- **Pattern Matching** - Algebraic data types with exhaustive matching
- **Coroutines** - Generators and async iteration

## Example

```ylc
# Fibonacci with pattern matching
let fib = fn x ->
  match x with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (x - 1)) + (fib (x - 2))
;;

# First-class functions
let map = fn f list ->
  match list with
  | [] -> []
  | h :: t -> (f h) :: (map f t)
;;
```

## Components

YALCE consists of three main components:

1. **Language Frontend** - Parser, type checker, and compiler
2. **Audio Engine** - Real-time audio graph processing in C
3. **GUI Framework** - OpenGL-based user interface

