# YALCE Examples

Practical code examples demonstrating YALCE features.

## Table of Contents

- [Basic Examples](#basic-examples)
- [Data Structures](#data-structures)
- [Functional Programming](#functional-programming)
- [Pattern Matching](#pattern-matching)
- [Recursion](#recursion)
- [Audio Examples](#audio-examples)

## Basic Examples

### Hello World

```ylc
let message = "Hello, YALCE!";;
print message;;
```

### Simple Calculator

```ylc
let add = fn x y -> x + y;;
let sub = fn x y -> x - y;;
let mul = fn x y -> x * y;;
let div = fn x y ->
  if y == 0 then
    None
  else
    Some (x / y)
;;

print (add 5 3);;     # 8
print (mul 4 7);;     # 28
print (div 10 2);;    # Some 5
print (div 10 0);;    # None
```

### FizzBuzz

```ylc
let fizzbuzz = fn n ->
  if n % 15 == 0 then
    "FizzBuzz"
  else if n % 3 == 0 then
    "Fizz"
  else if n % 5 == 0 then
    "Buzz"
  else
    string_of_int n
;;

let rec run_fizzbuzz = fn i max ->
  if i > max then
    ()
  else (
    print (fizzbuzz i);
    run_fizzbuzz (i + 1) max
  )
;;

run_fizzbuzz 1 100;;
```

## Data Structures

### Working with Lists

```ylc
# List construction
let numbers = [1, 2, 3, 4, 5];;
let more_numbers = 0 :: numbers;;

# List length
let rec length = fn list ->
  match list with
  | [] -> 0
  | _ :: t -> 1 + length t
;;

# List sum
let rec sum = fn list ->
  match list with
  | [] -> 0
  | h :: t -> h + sum t
;;

# List reverse
let reverse = fn list ->
  let rec aux = fn acc list ->
    match list with
    | [] -> acc
    | h :: t -> aux (h :: acc) t
  in
  aux [] list
;;

print (length numbers);;    # 5
print (sum numbers);;       # 15
print (reverse numbers);;   # [5, 4, 3, 2, 1]
```

### Binary Tree

```ylc
type Tree a =
  | Leaf
  | Node a (Tree a) (Tree a);

# Insert into BST
let rec insert = fn value tree ->
  match tree with
  | Leaf -> Node value Leaf Leaf
  | Node v left right ->
      if value < v then
        Node v (insert value left) right
      else if value > v then
        Node v left (insert value right)
      else
        tree
;;

# Search in BST
let rec contains = fn value tree ->
  match tree with
  | Leaf -> false
  | Node v left right ->
      if value == v then
        true
      else if value < v then
        contains value left
      else
        contains value right
;;

# In-order traversal
let rec inorder = fn tree ->
  match tree with
  | Leaf -> []
  | Node v left right ->
      (inorder left) ++ [v] ++ (inorder right)
;;

# Example usage
let tree = Leaf;;
let tree = insert 5 tree;;
let tree = insert 3 tree;;
let tree = insert 7 tree;;
let tree = insert 1 tree;;

print (contains 3 tree);;   # true
print (contains 4 tree);;   # false
print (inorder tree);;      # [1, 3, 5, 7]
```

## Functional Programming

### Map, Filter, Reduce

```ylc
# Map: Transform each element
let rec map = fn f list ->
  match list with
  | [] -> []
  | h :: t -> (f h) :: (map f t)
;;

# Filter: Keep elements matching predicate
let rec filter = fn pred list ->
  match list with
  | [] -> []
  | h :: t ->
      if pred h then
        h :: (filter pred t)
      else
        filter pred t
;;

# Fold: Reduce list to single value
let rec fold_left = fn f acc list ->
  match list with
  | [] -> acc
  | h :: t -> fold_left f (f acc h) t
;;

# Examples
let numbers = [1, 2, 3, 4, 5];;

let doubled = map (fn x -> x * 2) numbers;;
# [2, 4, 6, 8, 10]

let evens = filter (fn x -> x % 2 == 0) numbers;;
# [2, 4]

let sum = fold_left (+) 0 numbers;;
# 15

let product = fold_left ( * ) 1 numbers;;
# 120
```

### Function Composition

```ylc
# Compose two functions
let compose = fn f g x -> f (g x);;

# Pipeline (left-to-right composition)
let pipeline = fn f g x -> g (f x);;

# Example: double then add 1
let double = fn x -> x * 2;;
let add1 = fn x -> x + 1;;

let double_then_add1 = pipeline double add1;;
print (double_then_add1 5);;  # 11

let add1_then_double = pipeline add1 double;;
print (add1_then_double 5);;  # 12
```

### Currying and Partial Application

```ylc
let add = fn x y -> x + y;;

# Partial application
let add5 = add 5;;
let add10 = add 10;;

print (add5 3);;    # 8
print (add10 7);;   # 17

# Curried multiply
let multiply = fn x -> fn y -> x * y;;
let double = multiply 2;;
let triple = multiply 3;;

print (double 7);;  # 14
print (triple 7);;  # 21
```

## Pattern Matching

### Option Type

```ylc
type Option a = Some a | None;

let safe_div = fn a b ->
  if b == 0 then None
  else Some (a / b)
;;

let unwrap_or = fn default opt ->
  match opt with
  | Some x -> x
  | None -> default
;;

let map_option = fn f opt ->
  match opt with
  | Some x -> Some (f x)
  | None -> None
;;

let result = safe_div 10 2;;
let value = unwrap_or 0 result;;
print value;;  # 5

let doubled = map_option (fn x -> x * 2) result;;
# Some 10
```

### Result Type

```ylc
type Result err ok =
  | Ok ok
  | Error err;

let parse_int = fn str ->
  # Simplified - actual implementation would parse
  if str == "123" then
    Ok 123
  else
    Error "Invalid number"
;;

let map_result = fn f result ->
  match result with
  | Ok value -> Ok (f value)
  | Error err -> Error err
;;

let result = parse_int "123";;
let doubled = map_result (fn x -> x * 2) result;;
# Ok 246
```

### List Patterns

```ylc
# Get first and second elements
let first_two = fn list ->
  match list with
  | [] -> None
  | [_] -> None
  | x :: y :: _ -> Some (x, y)
;;

# Split at first element
let split_head = fn list ->
  match list with
  | [] -> None
  | h :: t -> Some (h, t)
;;

# Pattern matching multiple elements
let describe = fn list ->
  match list with
  | [] -> "empty"
  | [_] -> "one element"
  | [_, _] -> "two elements"
  | _ :: _ :: _ :: _ -> "many elements"
;;
```

## Recursion

### Factorial

```ylc
let rec factorial = fn n ->
  match n with
  | 0 -> 1
  | 1 -> 1
  | _ -> n * (factorial (n - 1))
;;

print (factorial 5);;  # 120
```

### Fibonacci

```ylc
# Naive recursive
let rec fib = fn n ->
  match n with
  | 0 -> 0
  | 1 -> 1
  | _ -> (fib (n - 1)) + (fib (n - 2))
;;

# Tail-recursive (more efficient)
let fib_fast = fn n ->
  let rec aux = fn i a b ->
    if i == n then a
    else aux (i + 1) b (a + b)
  in
  aux 0 0 1
;;

print (fib 10);;       # 55
print (fib_fast 10);;  # 55
```

### Greatest Common Divisor

```ylc
let rec gcd = fn a b ->
  if b == 0 then
    a
  else
    gcd b (a % b)
;;

print (gcd 48 18);;  # 6
```

### Quicksort

```ylc
let rec quicksort = fn list ->
  match list with
  | [] -> []
  | pivot :: rest ->
      let smaller = filter (fn x -> x < pivot) rest in
      let greater = filter (fn x -> x >= pivot) rest in
      (quicksort smaller) ++ [pivot] ++ (quicksort greater)
;;

let unsorted = [3, 1, 4, 1, 5, 9, 2, 6];;
print (quicksort unsorted);;
# [1, 1, 2, 3, 4, 5, 6, 9]
```

## Audio Examples

### Simple Oscillator

```ylc
# Sine wave oscillator
let sin_osc = fn freq phase ->
  sin (phase * 2.0 * 3.14159 * freq)
;;

# Generate 440Hz tone (A4)
let frequency = 440.0;;
let sample_rate = 44100.0;;

let generate_tone = fn duration ->
  let num_samples = int_of_float (duration * sample_rate) in
  let rec generate = fn i acc ->
    if i >= num_samples then
      reverse acc
    else
      let t = (float_of_int i) / sample_rate in
      let sample = sin_osc frequency t in
      generate (i + 1) (sample :: acc)
  in
  generate 0 []
;;

let tone = generate_tone 1.0;;  # 1 second of 440Hz
```

### Audio Processing Chain

```ylc
# Simple effects chain
let amplify = fn gain signal ->
  map (fn x -> x * gain) signal
;;

let clip = fn threshold signal ->
  map (fn x ->
    if x > threshold then threshold
    else if x < -threshold then -threshold
    else x
  ) signal
;;

# Process audio
let dry_signal = generate_tone 0.5;;
let amplified = amplify 2.0 dry_signal;;
let clipped = clip 0.9 amplified;;
```

### ADSR Envelope

```ylc
type ADSRPhase =
  | Attack
  | Decay
  | Sustain
  | Release;

let adsr_envelope = fn attack decay sustain release ->
  fn time duration ->
    let a_time = attack in
    let d_time = a_time + decay in
    let s_time = duration - release in
    let r_time = duration in

    if time < a_time then
      # Attack phase
      time / a_time
    else if time < d_time then
      # Decay phase
      1.0 - ((time - a_time) / decay) * (1.0 - sustain)
    else if time < s_time then
      # Sustain phase
      sustain
    else if time < r_time then
      # Release phase
      sustain * (1.0 - (time - s_time) / release)
    else
      0.0
;;

let envelope = adsr_envelope 0.1 0.2 0.7 0.3;;
```

## See Also

- [Language Reference](ylc-ref.md) - Complete syntax guide
- [Type System](type-system.md) - Understanding types
- [Audio Engine](audio-engine.md) - Real-time audio processing
- [API Reference](api-reference.md) - Standard library functions
