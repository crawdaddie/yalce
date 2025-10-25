# Type System

YALCE uses Hindley-Milner type inference, which means you rarely need to write type annotations—the compiler infers types automatically.

## Type Inference

The compiler automatically determines types:

```ylc
let x = 42;;
# x : int

let double = fn x -> x * 2;;
# double : int -> int

let id = fn x -> x;;
# id : 'a -> 'a (polymorphic)
```

## Polymorphism

Functions can work with any type:

```ylc
# Generic identity function
let id = fn x -> x;;
# id : 'a -> 'a

id 5;;        # 5 : int
id "hello";;  # "hello" : string
id [1, 2];;   # [1, 2] : [int]
```

### Type Variables

Type variables are written with a leading apostrophe:

- `'a`, `'b`, `'c` - Generic type variables
- `['a]` - List of any type
- `'a -> 'b` - Function from any type to any type

## Algebraic Data Types (ADTs)

### Simple Enums

```ylc
type Color = Red | Green | Blue;

let is_primary = fn color ->
  match color with
  | Red -> true
  | Green -> true
  | Blue -> true
;;
```

### Parameterized Types

```ylc
type Option a = Some a | None;

let safe_head = fn list ->
  match list with
  | [] -> None
  | h :: _ -> Some h
;;
# safe_head : ['a] -> Option 'a
```

### Recursive Types

```ylc
type Tree a =
  | Leaf
  | Node a (Tree a) (Tree a);

let singleton = fn value -> Node value Leaf Leaf;;
# singleton : 'a -> Tree 'a
```

### Multiple Type Parameters

```ylc
type Result err ok =
  | Ok ok
  | Error err;

let divide = fn a b ->
  if b == 0 then
    Error "Division by zero"
  else
    Ok (a / b)
;;
# divide : int -> int -> Result string int
```

## Function Types

### Simple Functions

```ylc
let add = fn x y -> x + y;;
# add : int -> int -> int
```

### Higher-Order Functions

```ylc
let apply = fn f x -> f x;;
# apply : ('a -> 'b) -> 'a -> 'b

let compose = fn f g x -> f (g x);;
# compose : ('b -> 'c) -> ('a -> 'b) -> 'a -> 'c
```

### Currying

All multi-argument functions are curried:

```ylc
let add = fn x y -> x + y;;
# add : int -> int -> int
# Equivalent to: int -> (int -> int)

let add5 = add 5;;
# add5 : int -> int
```

## List Types

```ylc
[1, 2, 3];;
# [int]

["a", "b", "c"];;
# [string]

[[1, 2], [3, 4]];;
# [[int]]

[];;
# ['a] (polymorphic empty list)
```

## Tuple Types

```ylc
(1, "hello");;
# (int, string)

(1, 2, 3);;
# (int, int, int)

(true, [1, 2], "test");;
# (bool, [int], string)
```

## Type Aliases

Create names for complex types:

```ylc
type Point = (float, float);
type UserId = int;
type Callback = int -> unit;

let distance = fn (x1, y1) (x2, y2) ->
  sqrt ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))
;;
# distance : Point -> Point -> float
```

## Type Constraints

### Numeric Types

YALCE distinguishes between `int` and `float`:

```ylc
1 + 2;;        # 3 : int
1.0 + 2.0;;    # 3.0 : float

# Error: Type mismatch
1 + 2.0;;      # Cannot mix int and float
```

Convert between types:

```ylc
float_of_int 42;;    # 42.0 : float
int_of_float 3.14;;  # 3 : int
```

### List Homogeneity

Lists must contain elements of the same type:

```ylc
[1, 2, 3];;        # OK: [int]
["a", "b"];;       # OK: [string]

# Error: Type mismatch
[1, "hello"];;     # Cannot mix types
```

## Pattern Matching and Types

Pattern matching must be exhaustive:

```ylc
type Option a = Some a | None;

# Warning: Non-exhaustive pattern
let unwrap = fn opt ->
  match opt with
  | Some x -> x
  # Missing: | None -> ...
;;
```

The compiler ensures all cases are covered:

```ylc
# Complete pattern matching
let unwrap_or = fn default opt ->
  match opt with
  | Some x -> x
  | None -> default
;;
```

## Type Inference Examples

### Example 1: Map Function

```ylc
let map = fn f list ->
  match list with
  | [] -> []
  | h :: t -> (f h) :: (map f t)
;;
```

Inferred type:
```
map : ('a -> 'b) -> ['a] -> ['b]
```

Breakdown:
- `f` is a function from type `'a` to type `'b`
- `list` is a list of `'a`
- Result is a list of `'b`

### Example 2: Filter Function

```ylc
let filter = fn pred list ->
  match list with
  | [] -> []
  | h :: t ->
      if pred h then
        h :: (filter pred t)
      else
        filter pred t
;;
```

Inferred type:
```
filter : ('a -> bool) -> ['a] -> ['a]
```

### Example 3: Fold Function

```ylc
let fold_left = fn f acc list ->
  match list with
  | [] -> acc
  | h :: t -> fold_left f (f acc h) t
;;
```

Inferred type:
```
fold_left : ('a -> 'b -> 'a) -> 'a -> ['b] -> 'a
```

## Type Errors

### Common Type Errors

**Type Mismatch**
```ylc
let add = fn x y -> x + y;;
add 1 "hello";;
# Error: Expected int but got string
```

**Infinite Type**
```ylc
let infinite = fn x -> x x;;
# Error: Cannot construct infinite type: 'a = 'a -> 'b
```

**Missing Pattern**
```ylc
let head = fn list ->
  match list with
  | h :: t -> h
;;
# Warning: Pattern match is not exhaustive
# Missing case: []
```

## Advanced Type Features

### Existential Types (Planned)

```ylc
# Abstract types
type Stack a;

val empty : Stack 'a
val push : 'a -> Stack 'a -> Stack 'a
val pop : Stack 'a -> Option ('a, Stack 'a)
```

### Type Classes (Planned)

```ylc
# Planned feature for ad-hoc polymorphism
class Eq a where
  eq : a -> a -> bool
end

instance Eq int where
  eq = fn x y -> x == y
end
```

### GADTs (Planned)

```ylc
# Generalized Algebraic Data Types
type Expr a where
  | IntLit : int -> Expr int
  | BoolLit : bool -> Expr bool
  | Add : Expr int -> Expr int -> Expr int
  | If : Expr bool -> Expr a -> Expr a -> Expr a
end
```

## Type Annotations (Optional)

You can add type annotations for documentation:

```ylc
# Function with type annotation
let add : int -> int -> int = fn x y -> x + y;;

# Type annotation in let binding
let (x : int) = 42;;

# Explicit type parameters
let id : 'a -> 'a = fn x -> x;;
```

## Best Practices

1. **Let the Compiler Infer** - Avoid unnecessary type annotations
2. **Use Type Aliases** - Make complex types more readable
3. **Match Exhaustively** - Handle all pattern cases
4. **Leverage Polymorphism** - Write generic, reusable functions
5. **Use ADTs** - Model your domain with algebraic data types

## See Also

- [Language Reference](ylc-ref.md) - Complete language syntax
- [Examples](examples.md) - Type system in practice
