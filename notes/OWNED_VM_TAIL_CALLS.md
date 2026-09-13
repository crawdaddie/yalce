# Owned VM Tail Calls

## Problem

An interpreter runner that threads an owned VM record can look tail-recursive in
source but fail to become a real tail call after Perceus inserts cleanup.

Example shape:

```ylc
type Vm = (
  pc: Int,
  rb: Int,
  prog: Array of Int,
  event: VmEvent
);

let run_vm = fn vm ->
  let nvm = step_vm vm;

  match nvm.event with
  | Continue -> run_vm nvm
  | Output v -> (
    print `{v}\n`;
    run_vm (Vm nvm.pc nvm.rb nvm.prog Continue)
  )
  | Halted -> ()
;;
```

The recursive call is syntactically last in the branch. The MIR is not.

## MIR Symptom

Perceus keeps the owned `nvm` alive across the recursive call, then drops it
afterwards:

```text
%7 = call $run_vm(%2)
%65 = drop %2
br match.cont
```

That means the call is not in tail position in MIR:

```text
call run_vm
drop old_vm
return
```

The same happens when constructing a fresh VM:

```text
%40 = construct.tuple { pc, rb, prog, Continue }
drop %2
%41 = call $run_vm(%40)
drop %40
br match.cont
```

## IR Symptom

LLVM may mark some calls as `tail`, but the function still contains post-call
cleanup and a join block:

```llvm
%call1 = tail call @run_vm(...)
; drop old program / VM fields
br label %match.cont

match.cont:
  ret void
```

For branches with borrowed event payloads, such as `NeedsInput`, lowering may
emit a normal call:

```llvm
call void @run_vm(...)
; drop borrowed slice / owning aggregate
```

This grows the C stack during long Intcode runs. AoC 2019 Day 09 input `2`
segfaulted on the normal stack but passed with `ulimit -s unlimited`.

## Why

The owned VM record mixes:

- scalar machine state: `pc`, `rb`
- owned heap state: `prog`
- transient event state: `event`

`step_vm` returns a new owned aggregate. Matching on `event` borrows fields from
that aggregate. The recursive call borrows or reconstructs from those fields.
Perceus must then preserve and release the aggregate around the call.

Source tail position is not enough. Ownership cleanup must also be before the
recursive call, or transferred into the call.

## Direct Runner Workaround

Thread scalar state directly:

```ylc
let run_pc = fn pc rb arr ->
  let pcn, rbn, ev = step_direct pc rb arr;

  match ev with
  | Continue -> run_pc pcn rbn arr
  | Output v -> (
    print `{v}\n`;
    run_pc pcn rbn arr
  )
  | Halted -> ()
;;
```

This avoids constructing `Vm` per instruction. The hot path lowers to:

```llvm
tail call void @run_pc(...)
```

For Day 09 this fixed the normal-stack crash.

## Coroutine Compatibility

The owned `Vm` shape is still useful for coroutine-style interpreters:

```text
step_vm : Vm -> Vm
```

It supports schedulers that need to stop at:

- `NeedsInput`
- `Output`
- `Halted`

AoC Day 07 feedback mode needs that behavior.

For speed, use a scalar yielding runner instead:

```text
run_amp(pc, rb, prog, phase, signal, phase_used)
  -> (pc, rb, phase_used, halted, output)
```

It keeps the scheduler coroutine-ish without returning an owned `Vm`.

## Compiler Work

Possible fixes:

1. Recognize self-recursive tail calls before Perceus cleanup.
2. Move final drops before the recursive call when safe.
3. Consume the owned aggregate into the recursive call instead of borrowing it.
4. Add a MIR tail-call form that owns explicit cleanup placement.
5. Lower eligible self tail calls to a loop before RC insertion.

The key check:

```text
recursive call must be followed only by control transfer,
not by drop/dup/free work.
```

## Test Case

Use AoC 2019 Day 09 input `2`.

Expected:

```text
got output value: 35920
```

Bad owned-VM runner:

```text
segfaults with normal stack
passes with unlimited stack
```

Direct scalar runner:

```text
passes with normal stack
```
