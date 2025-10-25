# YALCE Language Reference

Complete reference for the YALCE programming language syntax and features.

## Table of Contents

- [Basic Syntax](#basic-syntax)
- [Types](#types)
- [Functions](#functions)
- [Pattern Matching](#pattern-matching)
- [Data Structures](#data-structures)
- [Modules](#modules)
- [Coroutines](#coroutines)
- [Operators](#operators)

## Basic Syntax

### Comments

```ylc
# Single-line comment

(* Multi-line
   comment *)
```

### Let Bindings

```ylc
let x = 42;;
let name = "YALCE";;
let pi = 3.14159;;
```

### Statements

Statements are terminated with `;;`:

```ylc
let x = 1;;
let y = 2;;
print (x + y);;
```

## Types

YALCE uses Hindley-Milner type inference, so type annotations are optional.

### Primitive Types

```ylc
# Integer
let count = 42;;
# count : int

# Float
let temperature = 98.6;;
# temperature : float

# String
let message = "Hello";;
# message : string

# Boolean
let flag = true;;
# flag : bool

# Unit (like void)
let nothing = ();;
# nothing : unit
```

### Type Aliases

```ylc
type UserId = int;
type Point = (float, float);
```

### Algebraic Data Types

```ylc
# Simple enum
type Color = Red | Green | Blue;

# Enum with data
type Option a = Some a | None;

# Complex ADT
type Tree a =
  | Leaf
  | Node a (Tree a) (Tree a);
```

### Polymorphic Types

```ylc
# Generic identity function
let id = fn x -> x;;
# id : 'a -> 'a

# Generic list functions
let map = fn f list ->
  match list with
  | [] -> []
  | h :: t -> (f h) :: (map f t)
;;
# map : ('a -> 'b) -> ['a] -> ['b]
```

## Functions

### Function Definition

```ylc
# Single parameter
let square = fn x -> x * x;;

# Multiple parameters
let add = fn x y -> x + y;;

# Multi-line function
let factorial = fn n ->
  match n with
  | 0 -> 1
  | 1 -> 1
  | _ -> n * (factorial (n - 1))
;;
```

### Anonymous Functions (Lambdas)

```ylc
let double = fn x -> x * 2;;

# Immediately invoked
(fn x -> x + 1) 5;;  # Returns 6
```

### Currying

Functions are automatically curried:

```ylc
let add = fn x y -> x + y;;

# Partial application
let add5 = add 5;;
add5 10;;  # Returns 15

# Can also write as:
let add = fn x -> fn y -> x + y;;
```

### Recursive Functions

Functions can call themselves:

```ylc
let fib = fn n ->
  match n with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (n - 1)) + (fib (n - 2))
;;
```

### Higher-Order Functions

```ylc
# Function that takes a function
let twice = fn f x -> f (f x);;

let add1 = fn x -> x + 1;;
twice add1 5;;  # Returns 7

# Function that returns a function
let make_adder = fn n ->
  fn x -> x + n
;;

let add10 = make_adder 10;;
add10 5;;  # Returns 15
```

## Pattern Matching

### Basic Match Expression

```ylc
let classify = fn n ->
  match n with
  | 0 -> "zero"
  | 1 -> "one"
  | _ -> "many"
;;
```

### Matching on ADTs

```ylc
type Option a = Some a | None;

let unwrap_or = fn default opt ->
  match opt with
  | Some value -> value
  | None -> default
;;
```

### List Patterns

```ylc
let head_opt = fn list ->
  match list with
  | [] -> None
  | h :: t -> Some h
;;

# Multiple elements
let first_two = fn list ->
  match list with
  | [] -> None
  | [x] -> None
  | x :: y :: rest -> Some (x, y)
;;
```

### Tuple Patterns

```ylc
let swap = fn tuple ->
  match tuple with
  | (a, b) -> (b, a)
;;
```

### Nested Patterns

```ylc
type Tree a = Leaf | Node a (Tree a) (Tree a);

let depth = fn tree ->
  match tree with
  | Leaf -> 0
  | Node _ Leaf Leaf -> 1
  | Node _ left right ->
      1 + max (depth left) (depth right)
;;
```

## Data Structures

### Lists

```ylc
# List literal
let numbers = [1, 2, 3, 4, 5];;

# Empty list
let empty = [];;

# Cons operator
let list = 1 :: 2 :: 3 :: [];;

# List concatenation
let combined = [1, 2] ++ [3, 4];;
```

### Tuples

```ylc
# Pair
let point = (3.0, 4.0);;

# Triple
let rgb = (255, 128, 0);;

# Accessing with pattern matching
let (x, y) = point;;
```

### Records (Planned)

```ylc
# Record syntax (if implemented)
type Person = { name: string, age: int };

let person = { name: "Alice", age: 30 };
```

### Arrays

```ylc
# Fixed-size arrays for audio processing
let buffer = Array.make 1024 0.0;;
Array.set buffer 0 1.0;;
let value = Array.get buffer 0;;
```

## Modules

### Module Definition

```ylc
# math_utils.ylc
let square = fn x -> x * x;;
let cube = fn x -> x * x * x;;

export square, cube;
```

### Importing Modules

```ylc
import MathUtils;

let result = MathUtils.square 5;;
```

### Test Modules

```ylc
let test = module () ->
  let test_addition = 2 + 2 == 4;
  let test_square = square 5 == 25;
;
```

Run with:

```bash
ylc --test myfile.ylc
```

## Coroutines

YALCE supports coroutines for generators and iteration:

### Generator Functions

```ylc
let range = gen fn start end ->
  let current = ref start;
  while !current < end do
    yield !current;
    current := !current + 1;
  done
;;

# Using a generator
let nums = range 0 10;
for x in nums do
  print x;
done
```

### Async/Await Pattern

```ylc
let process_items = gen fn items ->
  for item in items do
    let result = process item;
    yield result;
  done
;;
```

## Operators

### Arithmetic Operators

```ylc
+ # Addition
- # Subtraction
* # Multiplication
/ # Division
% # Modulo
```

### Comparison Operators

```ylc
== # Equal
!= # Not equal
<  # Less than
<= # Less than or equal
>  # Greater than
>= # Greater than or equal
```

### Logical Operators

```ylc
&& # Logical AND
|| # Logical OR
!  # Logical NOT
```

### List Operators

```ylc
::  # Cons (prepend to list)
++  # List concatenation
```

### String Operators

```ylc
++  # String concatenation
"Hello" ++ " " ++ "World"  # "Hello World"
```

### Pipeline Operator

```ylc
|> # Forward pipe (planned)

# Instead of: f (g (h x))
# Write: x |> h |> g |> f
```

## Control Flow

### If Expressions

```ylc
let abs = fn x ->
  if x < 0 then
    -x
  else
    x
;;
```

### While Loops

```ylc
let count_down = fn n ->
  let i = ref n;
  while !i > 0 do
    print !i;
    i := !i - 1;
  done
;;
```

### For Loops

```ylc
for i in [1, 2, 3, 4, 5] do
  print i;
done
```

## References (Mutable State)

```ylc
# Create a reference
let counter = ref 0;;

# Read a reference
let value = !counter;;

# Update a reference
counter := !counter + 1;;
```

## Built-in Functions

### I/O Functions

```ylc
print value          # Print to stdout
print_string str     # Print string
print_int n          # Print integer
print_float f        # Print float
```

### List Functions

```ylc
List.length [1, 2, 3]        # 3
List.map (fn x -> x * 2) [1, 2, 3]  # [2, 4, 6]
List.filter (fn x -> x > 0) [-1, 0, 1]  # [1]
List.fold_left (+) 0 [1, 2, 3]  # 6
```

### Math Functions

```ylc
sin 1.0
cos 1.0
sqrt 2.0
abs (-5)
max 3 7
min 3 7
```

### String Functions

```ylc
String.length "hello"          # 5
String.concat " " ["hello", "world"]  # "hello world"
String.sub "hello" 0 3        # "hel"
```

## FFI (Foreign Function Interface)

Call C functions from YALCE:

```ylc
external malloc : int -> ptr = "malloc";
external free : ptr -> unit = "free";

# Audio processing
external sin_osc : float -> float = "sin_osc";
```

## Syntax Cheat Sheet

```ylc
# Variables
let x = 42;;

# Functions
let f = fn x -> x + 1;;

# Pattern matching
match x with
| 0 -> "zero"
| _ -> "other"
;;

# Lists
let list = [1, 2, 3];;
let cons = 0 :: list;;

# Tuples
let pair = (1, 2);;

# Types
type Option a = Some a | None;

# Modules
import MyModule;
export f, g;

# Coroutines
let gen = gen fn () ->
  yield 1;
  yield 2;
;;
```

## See Also

- [Type System](type-system.md) - Deep dive into Hindley-Milner inference
- [Examples](examples.md) - More code examples
- [API Reference](api-reference.md) - Standard library documentation
