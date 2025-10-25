# API Reference

Complete reference for YALCE standard library and built-in functions.

## Table of Contents

- [Core Functions](#core-functions)
- [List Module](#list-module)
- [String Module](#string-module)
- [Array Module](#array-module)
- [Math Module](#math-module)
- [Audio Module](#audio-module)
- [I/O Functions](#io-functions)

## Core Functions

### Type Conversion

#### `int_of_float`
Convert float to integer (truncates).
```ylc
int_of_float : float -> int
```

**Example:**
```ylc
int_of_float 3.14;;  # 3
int_of_float 9.99;;  # 9
```

#### `float_of_int`
Convert integer to float.
```ylc
float_of_int : int -> float
```

**Example:**
```ylc
float_of_int 42;;  # 42.0
```

#### `string_of_int`
Convert integer to string.
```ylc
string_of_int : int -> string
```

**Example:**
```ylc
string_of_int 42;;  # "42"
```

#### `string_of_float`
Convert float to string.
```ylc
string_of_float : float -> string
```

**Example:**
```ylc
string_of_float 3.14;;  # "3.14"
```

### Comparison

#### `max`
Return maximum of two values.
```ylc
max : 'a -> 'a -> 'a
```

**Example:**
```ylc
max 5 10;;     # 10
max 3.2 2.9;;  # 3.2
```

#### `min`
Return minimum of two values.
```ylc
min : 'a -> 'a -> 'a
```

**Example:**
```ylc
min 5 10;;     # 5
min 3.2 2.9;;  # 2.9
```

### References

#### `ref`
Create a mutable reference.
```ylc
ref : 'a -> ref 'a
```

**Example:**
```ylc
let x = ref 0;;
```

#### `!` (dereference)
Read value from reference.
```ylc
(!) : ref 'a -> 'a
```

**Example:**
```ylc
let x = ref 42;;
!x;;  # 42
```

#### `:=` (assignment)
Update reference value.
```ylc
(:=) : ref 'a -> 'a -> unit
```

**Example:**
```ylc
let x = ref 0;;
x := 42;;
!x;;  # 42
```

## List Module

### `List.length`
Return length of list.
```ylc
List.length : ['a] -> int
```

**Example:**
```ylc
List.length [1, 2, 3];;  # 3
List.length [];;         # 0
```

### `List.map`
Apply function to each element.
```ylc
List.map : ('a -> 'b) -> ['a] -> ['b]
```

**Example:**
```ylc
List.map (fn x -> x * 2) [1, 2, 3];;
# [2, 4, 6]
```

### `List.filter`
Keep elements matching predicate.
```ylc
List.filter : ('a -> bool) -> ['a] -> ['a]
```

**Example:**
```ylc
List.filter (fn x -> x > 0) [-1, 0, 1, 2];;
# [1, 2]
```

### `List.fold_left`
Reduce list from left to right.
```ylc
List.fold_left : ('a -> 'b -> 'a) -> 'a -> ['b] -> 'a
```

**Example:**
```ylc
List.fold_left (+) 0 [1, 2, 3, 4];;
# 10
```

### `List.fold_right`
Reduce list from right to left.
```ylc
List.fold_right : ('a -> 'b -> 'b) -> ['a] -> 'b -> 'b
```

**Example:**
```ylc
List.fold_right (::) [1, 2, 3] [];;
# [1, 2, 3]
```

### `List.head`
Get first element (raises exception if empty).
```ylc
List.head : ['a] -> 'a
```

**Example:**
```ylc
List.head [1, 2, 3];;  # 1
```

### `List.tail`
Get all but first element.
```ylc
List.tail : ['a] -> ['a]
```

**Example:**
```ylc
List.tail [1, 2, 3];;  # [2, 3]
```

### `List.reverse`
Reverse a list.
```ylc
List.reverse : ['a] -> ['a]
```

**Example:**
```ylc
List.reverse [1, 2, 3];;
# [3, 2, 1]
```

### `List.concat`
Concatenate list of lists.
```ylc
List.concat : [['a]] -> ['a]
```

**Example:**
```ylc
List.concat [[1, 2], [3, 4], [5]];;
# [1, 2, 3, 4, 5]
```

### `List.nth`
Get element at index (0-based).
```ylc
List.nth : ['a] -> int -> 'a
```

**Example:**
```ylc
List.nth [10, 20, 30] 1;;  # 20
```

### `List.find`
Find first element matching predicate.
```ylc
List.find : ('a -> bool) -> ['a] -> Option 'a
```

**Example:**
```ylc
List.find (fn x -> x > 5) [1, 3, 7, 9];;
# Some 7
```

### `List.exists`
Check if any element matches predicate.
```ylc
List.exists : ('a -> bool) -> ['a] -> bool
```

**Example:**
```ylc
List.exists (fn x -> x > 10) [1, 2, 3];;
# false
```

### `List.for_all`
Check if all elements match predicate.
```ylc
List.for_all : ('a -> bool) -> ['a] -> bool
```

**Example:**
```ylc
List.for_all (fn x -> x > 0) [1, 2, 3];;
# true
```

## String Module

### `String.length`
Return string length.
```ylc
String.length : string -> int
```

**Example:**
```ylc
String.length "hello";;  # 5
```

### `String.concat`
Join strings with separator.
```ylc
String.concat : string -> [string] -> string
```

**Example:**
```ylc
String.concat " " ["hello", "world"];;
# "hello world"
```

### `String.sub`
Extract substring.
```ylc
String.sub : string -> int -> int -> string
```

**Example:**
```ylc
String.sub "hello" 1 3;;  # "ell"
```

### `String.split`
Split string by delimiter.
```ylc
String.split : string -> string -> [string]
```

**Example:**
```ylc
String.split "," "a,b,c";;
# ["a", "b", "c"]
```

### `String.trim`
Remove leading/trailing whitespace.
```ylc
String.trim : string -> string
```

**Example:**
```ylc
String.trim "  hello  ";;  # "hello"
```

### `String.to_upper`
Convert to uppercase.
```ylc
String.to_upper : string -> string
```

**Example:**
```ylc
String.to_upper "hello";;  # "HELLO"
```

### `String.to_lower`
Convert to lowercase.
```ylc
String.to_lower : string -> string
```

**Example:**
```ylc
String.to_lower "HELLO";;  # "hello"
```

## Array Module

### `Array.make`
Create array with initial value.
```ylc
Array.make : int -> 'a -> Array 'a
```

**Example:**
```ylc
Array.make 10 0.0;;  # Array of 10 zeros
```

### `Array.get`
Get element at index.
```ylc
Array.get : Array 'a -> int -> 'a
```

**Example:**
```ylc
let arr = Array.make 5 0;;
Array.get arr 2;;  # 0
```

### `Array.set`
Set element at index.
```ylc
Array.set : Array 'a -> int -> 'a -> unit
```

**Example:**
```ylc
let arr = Array.make 5 0;;
Array.set arr 2 42;;
```

### `Array.length`
Return array length.
```ylc
Array.length : Array 'a -> int
```

**Example:**
```ylc
let arr = Array.make 10 0;;
Array.length arr;;  # 10
```

### `Array.copy`
Create a copy of array.
```ylc
Array.copy : Array 'a -> Array 'a
```

### `Array.fill`
Fill array with value.
```ylc
Array.fill : Array 'a -> 'a -> unit
```

**Example:**
```ylc
let arr = Array.make 5 0;;
Array.fill arr 1;;  # [1, 1, 1, 1, 1]
```

## Math Module

### Trigonometric Functions

#### `sin`
Sine function.
```ylc
sin : float -> float
```

#### `cos`
Cosine function.
```ylc
cos : float -> float
```

#### `tan`
Tangent function.
```ylc
tan : float -> float
```

#### `asin`
Arcsine function.
```ylc
asin : float -> float
```

#### `acos`
Arccosine function.
```ylc
acos : float -> float
```

#### `atan`
Arctangent function.
```ylc
atan : float -> float
```

#### `atan2`
Two-argument arctangent.
```ylc
atan2 : float -> float -> float
```

### Exponential and Logarithmic

#### `exp`
Exponential function (e^x).
```ylc
exp : float -> float
```

#### `log`
Natural logarithm.
```ylc
log : float -> float
```

#### `log10`
Base-10 logarithm.
```ylc
log10 : float -> float
```

#### `pow`
Power function.
```ylc
pow : float -> float -> float
```

**Example:**
```ylc
pow 2.0 8.0;;  # 256.0
```

#### `sqrt`
Square root.
```ylc
sqrt : float -> float
```

**Example:**
```ylc
sqrt 16.0;;  # 4.0
```

### Other Math Functions

#### `abs`
Absolute value.
```ylc
abs : int -> int
abs_float : float -> float
```

**Example:**
```ylc
abs (-5);;       # 5
abs_float (-3.14);;  # 3.14
```

#### `floor`
Round down to integer.
```ylc
floor : float -> float
```

#### `ceil`
Round up to integer.
```ylc
ceil : float -> float
```

#### `round`
Round to nearest integer.
```ylc
round : float -> float
```

### Constants

```ylc
Math.pi;;     # 3.14159265...
Math.e;;      # 2.71828182...
```

## Audio Module

See [Audio Engine](audio-engine.md) for complete audio API documentation.

### Basic Functions

```ylc
Audio.init : int -> int -> unit
Audio.start : unit
Audio.stop : unit
Audio.sin_osc : float -> Node
Audio.saw_osc : float -> Node
Audio.square_osc : float -> Node
Audio.lpf : float -> float -> Node -> Node
Audio.hpf : float -> float -> Node -> Node
```

## I/O Functions

### `print`
Print value to stdout.
```ylc
print : 'a -> unit
```

**Example:**
```ylc
print "Hello";;
print 42;;
print [1, 2, 3];;
```

### `print_string`
Print string without newline.
```ylc
print_string : string -> unit
```

### `print_int`
Print integer.
```ylc
print_int : int -> unit
```

### `print_float`
Print float.
```ylc
print_float : float -> unit
```

### `read_line`
Read line from stdin.
```ylc
read_line : unit -> string
```

**Example:**
```ylc
let input = read_line ();;
print ("You entered: " ++ input);;
```

### File I/O (Planned)

```ylc
open_in : string -> file_handle
open_out : string -> file_handle
close_in : file_handle -> unit
close_out : file_handle -> unit
read_line_from : file_handle -> string
write_to : file_handle -> string -> unit
```

## See Also

- [Language Reference](ylc-ref.md) - Language syntax
- [Examples](examples.md) - Code examples
- [Audio Engine](audio-engine.md) - Audio processing
