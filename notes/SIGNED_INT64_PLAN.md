# Signed Int64 Plan

## Problem

The language currently has `Uint64`, but no native signed `Int64`.

AoC 2019 Day 09 exposed the gap. Intcode needs values like:

```text
1001, 26, -4, 26
```

The current workaround stores signed values in `Uint64` and calls extern
helpers:

```ylc
type Int64 = Uint64;

let int64_add = extern fn Uint64 -> Uint64 -> Uint64;
let int64_lt = extern fn Uint64 -> Uint64 -> Bool;
let int64_to_int = extern fn Uint64 -> Int;
```

That works, but it is not a real type. It leaks representation everywhere.

## Goal

Add first-class signed 64-bit integers:

```ylc
let x = -4i64;
let y = 1125899906842624i64;
```

with normal arithmetic, comparison, literals, arrays, printing, parsing, and
extern ABI support.

## Type System

Add a primitive type:

```c
T_INT64
```

or equivalent in the existing primitive type enum.

It should be distinct from:

```text
Int    // current machine/default int, lowered as i32
Uint64 // unsigned 64-bit
```

Initial conversions:

```text
Int -> Int64
Int64 -> Int       explicit or checked
Uint64 -> Int64    explicit
Int64 -> Uint64    explicit
```

Do not make `Uint64` and `Int64` silently interchangeable.

## Literals

Support signed 64-bit literal suffixes:

```ylc
0i64
1i64
-1i64
1125899906842624i64
```

Parser work:

- recognize `i64` suffix
- store literal kind as signed 64-bit
- preserve negative literals as either:
  - unary negation over positive `i64`, or
  - signed literal payload

Prefer unary negation if that matches current `Int` behavior.

Range checks:

```text
min: -9223372036854775808
max:  9223372036854775807
```

## Builtin Operators

Add overloads for:

```text
+  : Int64 -> Int64 -> Int64
-  : Int64 -> Int64 -> Int64
*  : Int64 -> Int64 -> Int64
/  : Int64 -> Int64 -> Int64
%  : Int64 -> Int64 -> Int64
== : Int64 -> Int64 -> Bool
!= : Int64 -> Int64 -> Bool
<  : Int64 -> Int64 -> Bool
<= : Int64 -> Int64 -> Bool
>  : Int64 -> Int64 -> Bool
>= : Int64 -> Int64 -> Bool
```

Use signed LLVM operations:

```text
sdiv
srem
icmp slt/sle/sgt/sge
```

Addition, subtraction, and multiplication can use normal integer ops.

## MIR

Add an `Int64` MIR type mapping.

Constant support:

```text
const.int64
```

or reuse integer constants with explicit type if MIR already supports typed
integer constants.

Operations should either:

- reuse existing integer ops with operand type deciding width/signedness, or
- add explicit signed-width ops.

Prefer type-driven lowering if existing `Int` and `Uint64` already share op
kinds.

Important signed-sensitive ops:

```text
division
modulo
comparison
casts
```

## LLVM Lowering

Lower `Int64` to:

```llvm
i64
```

Signed-specific lowering:

```text
/  -> LLVMBuildSDiv
%  -> LLVMBuildSRem
<  -> LLVMIntSLT
<= -> LLVMIntSLE
>  -> LLVMIntSGT
>= -> LLVMIntSGE
```

Conversions:

```text
Int -> Int64       sext i32 to i64
Int64 -> Int       trunc i64 to i32
Uint64 -> Int64    bitcast/no-op at LLVM level, explicit in type system
Int64 -> Uint64    bitcast/no-op at LLVM level, explicit in type system
```

## Runtime

Add runtime helpers only for operations not handled by LLVM or builtins.

Likely needed:

```c
_String int64_to_string(int64_t x);
int64_t parse_int64(_String str);
```

Maybe names:

```c
_String str_int64(int64_t x);
int64_t int64_parse(_String str);
```

Use fixed-width C types:

```c
#include <stdint.h>
#include <inttypes.h>
```

Printing:

```c
snprintf(buf, sizeof(buf), "%" PRId64, x);
```

## Standard Library

Expose:

```ylc
let parse_int64 = extern fn String -> Int64;
```

Add `str` / interpolation support so:

```ylc
print `{x}\n`;
```

works for `Int64`.

## ABI

Extern ABI should map:

```ylc
Int64 <-> int64_t
```

and keep:

```ylc
Uint64 <-> uint64_t
```

Check:

- direct extern calls
- function pointers
- tuples/records containing `Int64`
- arrays of `Int64`
- variants carrying `Int64`

## Tests

Add focused tests before replacing the workaround.

### Literals

```ylc
print `{-1i64}\n`;
print `{1125899906842624i64}\n`;
```

### Arithmetic

```ylc
let a = -4i64;
let b = 10i64;
print `{a + b}\n`;  # 6
print `{a * b}\n`;  # -40
```

### Division And Modulo

```ylc
print `{-7i64 / 3i64}\n`;
print `{-7i64 % 3i64}\n`;
```

Expected behavior should match C/LLVM signed division truncation toward zero
unless the language specifies otherwise.

### Comparisons

```ylc
print `{-1i64 < 1i64}\n`;       # true
print `{1i64 < -1i64}\n`;       # false
print `{-1i64 < 1u64}\n`;       # should reject or require cast
```

### Arrays

```ylc
let xs = [|-1i64, 0i64, 1i64|];
print `{xs[0]}\n`;
```

### Extern ABI

```ylc
let echo_i64 = extern fn Int64 -> Int64;
print `{echo_i64 -4i64}\n`;
```

Runtime C:

```c
int64_t echo_i64(int64_t x) {
  return x;
}
```

### Intcode Regression

Rewrite `Int64code.ylc` to use:

```ylc
type Int64 = native Int64
```

or just direct `Int64`.

Remove helper calls:

```text
int64_add
int64_mul
int64_lt
int64_eq
int64_to_int
int64_str
```

Day 09 must still output:

```text
input 1 -> 3512778005
input 2 -> 35920
```

## Migration

1. Add type representation and parser support.
2. Add MIR constants and type printing.
3. Add LLVM lowering for constants, ops, comparisons, casts.
4. Add runtime parse/string helpers.
5. Add stdlib declarations.
6. Add tests.
7. Rewrite `Int64code.ylc` to native `Int64`.
8. Delete temporary `int64_*` bit-pattern helpers if no longer needed.

## Risks

### Mixed Signedness

Avoid implicit `Int64 <-> Uint64` conversion. It will hide bugs.

### Literal Defaulting

Large unsuffixed integer literals should not silently become `Int64` unless the
language already has numeric defaulting rules.

### `Int64 -> Int`

Truncation can break addresses. Keep it explicit where possible.

### Negative Literal Parsing

`-9223372036854775808i64` is awkward because the positive magnitude does not fit
in signed `int64_t`. Treat it as a special unary-negated literal or parse with a
wider temporary.

## Desired End State

Day 09 code should read like normal arithmetic:

```ylc
let get_operand = fn pc rb arr offset mode ->
  let p = arr[pc + offset];

  match mode with
  | 0 -> arr[p |> to_int]
  | 1 -> p
  | 2 -> arr[(rb + p) |> to_int]
;;
```

No fake `type Int64 = Uint64`.
No bit-pattern helper functions for basic arithmetic.
