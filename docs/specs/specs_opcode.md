# Opcode Specification

Stack-based VM. Opcodes are defined in `main/thingbot_vm/opcode.h` and executed in `main/thingbot_vm/vm.c`. Error codes are defined in `main/thingbot_vm/error.h`.

## Table of Contents

- [Conventions](#conventions)
- [Opcode Summary](#opcode-summary)
- [Control](#control)
  - [HLT](#hlt)
- [Stack](#stack)
  - [PSH](#psh)
- [Arithmetic](#arithmetic)
  - [ADD](#add), [SUB](#sub), [MUL](#mul)
- [Comparison](#comparison)
  - [GT](#gt), [GTE](#gte), [LT](#lt), [LTE](#lte), [EQ](#eq), [NEQ](#neq)
- [Locals](#locals)
  - [LOAD](#load), [STORE](#store)
- [Branching](#branching)
  - [JMP](#jmp), [JNZ](#jnz)
- [Calls (reserved)](#calls-reserved)
  - [INVOKE](#invoke), [IRETURN](#ireturn)
- [Errors](#errors)
  - [Error codes](#error-codes)
  - [Current behavior](#current-behavior)

## Conventions

- Program is an array of `int`. Each opcode occupies one slot; its operand (if any) occupies the next slot.
- Stack effect notation: `( before -- after )`, top of stack on the right.
- For binary operators, `a` is the value pushed first and `b` is the value pushed last (top of stack).
- Comparison results are `1` (true) or `0` (false).
- `VM_LOCALS` is the number of local slots; valid local indices are `0 .. VM_LOCALS - 1`.

## Opcode Summary

| Value | Mnemonic  | Category   | Operand        | Stack effect        | Status      |
|------:|-----------|------------|----------------|---------------------|-------------|
| 0     | `HLT`     | Control    | -              | `( -- )`            | Implemented |
| 1     | `PSH`     | Stack      | `value`        | `( -- value )`      | Implemented |
| 2     | `ADD`     | Arithmetic | -              | `( a b -- a+b )`    | Implemented |
| 3     | `SUB`     | Arithmetic | -              | `( a b -- a-b )`    | Implemented |
| 4     | `MUL`     | Arithmetic | -              | `( a b -- a*b )`    | Implemented |
| 5     | `LOAD`    | Locals     | `idx`          | `( -- local[idx] )` | Implemented |
| 6     | `STORE`   | Locals     | `idx`          | `( v -- )`          | Implemented |
| 7     | `JMP`     | Branching  | `target_ip`    | `( -- )`            | Implemented |
| 8     | `JNZ`     | Branching  | `target_ip`    | `( cond -- )`       | Implemented |
| 9     | `GT`      | Comparison | -              | `( a b -- a>b )`    | Implemented |
| 10    | `GTE`     | Comparison | -              | `( a b -- a>=b )`   | Implemented |
| 11    | `LT`      | Comparison | -              | `( a b -- a<b )`    | Implemented |
| 12    | `LTE`     | Comparison | -              | `( a b -- a<=b )`   | Implemented |
| 13    | `EQ`      | Comparison | -              | `( a b -- a==b )`   | Implemented |
| 14    | `NEQ`     | Comparison | -              | `( a b -- a!=b )`   | Implemented |
| 15    | `INVOKE`  | Calls      | -              | -                   | Reserved    |
| 16    | `IRETURN` | Calls      | -              | -                   | Reserved    |

## Control

### HLT

Stops the VM.

- Operand: none
- Stack effect: `( -- )`

## Stack

### PSH

Pushes an immediate value onto the stack.

- Operand: `value`
- Stack effect: `( -- value )`

## Arithmetic

Pop `b`, then `a`, push the result. Integer arithmetic, no overflow handling.

### ADD

- Stack effect: `( a b -- a+b )`

### SUB

- Stack effect: `( a b -- a-b )`

### MUL

- Stack effect: `( a b -- a*b )`

## Comparison

Pop `b`, then `a`, push `1` if the relation holds, else `0`.

### GT

- Stack effect: `( a b -- a>b )`

### GTE

- Stack effect: `( a b -- a>=b )`

### LT

- Stack effect: `( a b -- a<b )`

### LTE

- Stack effect: `( a b -- a<=b )`

### EQ

- Stack effect: `( a b -- a==b )`

### NEQ

- Stack effect: `( a b -- a!=b )`

## Locals

### LOAD

Pushes the value of a local slot onto the stack.

- Operand: `idx`
- Stack effect: `( -- local[idx] )`
- Error: `idx` out of range halts the VM.

### STORE

Pops the top of the stack into a local slot.

- Operand: `idx`
- Stack effect: `( v -- )`
- Error: `idx` out of range halts the VM.

## Branching

### JMP

Unconditional jump.

- Operand: `target_ip`
- Stack effect: `( -- )`
- Sets `ip = target_ip`.

### JNZ

Conditional jump. The condition is always popped, so loops do not grow the stack.

- Operand: `target_ip`
- Stack effect: `( cond -- )`
- Sets `ip = target_ip` if `cond != 0`; otherwise continues with the next instruction.

## Calls (reserved)

Declared in `opcode.h` but not handled by `vm_exec` yet; executing them currently falls into the bad-opcode error.

### INVOKE

Reserved for function calls. Semantics not defined yet.

### IRETURN

Reserved for returning from a call. Semantics not defined yet.

## Errors

### Error codes

Defined by the `Error` enum in `error.h`.

| Value | Name             | Meaning                                      |
|------:|------------------|----------------------------------------------|
| 0     | `SUCCESS`        | No error (0 is reserved for success)         |
| 1     | `INTERNAL`       | Internal implementation error of the VM      |
| 2     | `STACK_OVERFLOW` | Stack overflow                               |
| 3     | `OUT_OF_MEM`     | Out of memory                                |
| 4     | `UNKNOWN`        | Unknown error                                |

### Current behavior

`vm.c` does not return `Error` codes yet. On the conditions below it logs an error and halts (`running = false`):

| Condition       | Trigger                                                  |
|-----------------|----------------------------------------------------------|
| Bad opcode      | Opcode value with no handler (incl. reserved opcodes)    |
| Bad local index | `LOAD` / `STORE` with `idx` outside `0 .. VM_LOCALS - 1` |

Stack overflow/underflow and out-of-range jump targets are **not** checked, so `STACK_OVERFLOW` is never raised.
