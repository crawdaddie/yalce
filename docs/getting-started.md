# Getting Started with YALCE

This guide will help you install YALCE and write your first programs.

## Installation

### Option 1: Homebrew (macOS)

```bash
brew install crawdaddie/yalce/yalce
```

### Option 2: Build from Source

#### Prerequisites

- **LLVM** - For JIT compilation
- **readline** - For REPL line editing
- **libsoundio** - For audio I/O
- **fftw3** - For FFT operations
- **SDL2** and **SDL2_ttf** - For GUI (optional)

On macOS with Homebrew:

```bash
brew install llvm readline libsoundio fftw sdl2 sdl2_ttf
```

#### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/crawdaddie/yalce.git
   cd yalce
   ```

2. Create a `.env` file with paths to dependencies:
   ```bash
   LLVM_PATH=/opt/homebrew/opt/llvm
   READLINE_PREFIX=/opt/homebrew/opt/readline
   SDL2_PATH=/opt/homebrew/opt/sdl2
   SDL2_TTF_PATH=/opt/homebrew/opt/sdl2_ttf
   ```

   On macOS, you can use:
   ```bash
   echo "READLINE_PREFIX=$(brew --prefix readline)" > .env
   echo "LLVM_PATH=$(brew --prefix llvm)" >> .env
   ```

3. Build the compiler:
   ```bash
   make
   ```

   This creates the `ylc` executable in `build/ylc`.

4. (Optional) Add to your PATH:
   ```bash
   export PATH="$PATH:$(pwd)/build"
   ```

## Your First YALCE Program

### Hello World

Create a file `hello.ylc`:

```ylc
let message = "Hello, YALCE!";
print message;
```

Run it:

```bash
ylc hello.ylc
```

### Interactive REPL

Start the REPL:

```bash
ylc -i
```

Try some expressions:

```ylc
>> 1 + 2;;
3 : int

>> let double = fn x -> x * 2;;
double : int -> int

>> double 21;;
42 : int

>> let greet = fn name -> "Hello, " ++ name ++ "!";;
greet : string -> string

>> greet "World";;
"Hello, World!" : string
```

### Recursive Functions

Create `factorial.ylc`:

```ylc
let factorial = fn n ->
  match n with
  | 0 -> 1
  | 1 -> 1
  | _ -> n * (factorial (n - 1))
;;

print (factorial 5);
# Output: 120
```

Run it:

```bash
ylc factorial.ylc
```

### Pattern Matching with Lists

```ylc
let sum_list = fn list ->
  match list with
  | [] -> 0
  | head :: tail -> head + (sum_list tail)
;;

let numbers = [1, 2, 3, 4, 5];
print (sum_list numbers);
# Output: 15
```

### Working with Options

```ylc
type Option a = Some a | None;

let safe_div = fn a b ->
  if b == 0 then
    None
  else
    Some (a / b)
;;

let result = safe_div 10 2;

match result with
| Some value -> print value
| None -> print "Division by zero!"
;;
```

## REPL Commands

The REPL supports several commands:

- **Load a file**: `#use "filename.ylc";;`
- **Show type**: The REPL shows types after evaluation
- **Multi-line input**: Use `;;` to terminate an expression
- **Exit**: Press `Ctrl+D` or `Ctrl+C`

## Running Tests

YALCE includes a built-in test framework:

```ylc
# tests.ylc
let test = module () ->
  let test_addition = 2 + 2 == 4;
  let test_double = (fn x -> x * 2) 3 == 6;
;
```

Run tests:

```bash
ylc --test tests.ylc
```

## Next Steps

- [Language Reference](ylc-ref.md) - Learn all language features
- [Type System](type-system.md) - Understand type inference
- [Examples](examples.md) - Explore more code examples
- [Audio Engine](audio-engine.md) - Build audio applications

## Common Issues

### LLVM not found

If you get LLVM errors, ensure your `.env` file has the correct `LLVM_PATH`:

```bash
echo "LLVM_PATH=$(brew --prefix llvm)" > .env
```

### readline not found

On macOS, readline is keg-only:

```bash
echo "READLINE_PREFIX=$(brew --prefix readline)" >> .env
```

### Build errors

Try a clean build:

```bash
make clean
make
```

For debug builds:

```bash
make debug
```
