# Opcode Specification

Stack-based VM. Opcodes are defined in `main/vm/opcode.h` and dispatched in `main/vm/vm.c` to the handlers in `main/vm/handler.c`. Error codes are defined in `main/vm/error.h`. ThingBot-specific opcodes are specified in [opcode_board.md](opcode_board.md).

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
- [Calls](#calls)
  - [CALL](#call), [RET](#ret)
- [Timing](#timing)
  - [WAIT](#wait)
- [Errors](#errors)
  - [Error codes](#error-codes)
  - [Current behavior](#current-behavior)

## Conventions

- Program is an array of `int`. Each opcode occupies one slot; its operand (if any) occupies the next slot.
- Stack effect notation: `( before -- after )`, top of stack on the right.
- For binary operators, `a` is the value pushed first and `b` is the value pushed last (top of stack).
- Comparison results are `1` (true) or `0` (false).
- `VM_LOCALS` is the number of local slots; valid local indices are `0 .. VM_LOCALS - 1`.
- `VM_CALL_DEPTH` is the maximum number of nested calls.

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
| 15    | `CALL`    | Calls      | `target_ip`    | `( -- )`            | Implemented |
| 16    | `RET`     | Calls      | -              | `( -- )`            | Implemented |
| 17    | `WAIT`    | Timing     | -              | `( ms -- )`         | Implemented |

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

## Calls

Return addresses are kept on a separate call stack, up to `VM_CALL_DEPTH` deep. The operand stack and locals are shared between caller and callee: values pushed before `CALL` are visible to the callee, values left on the stack before `RET` are visible to the caller, and a callee writing `local[idx]` overwrites the caller's value.

### CALL

Calls a subroutine.

- Operand: `target_ip`
- Stack effect: `( -- )`
- Pushes the address of the next instruction onto the call stack, then sets `ip = target_ip`.
- Error: calling when the call stack already holds `VM_CALL_DEPTH` entries halts the VM.

### RET

Returns from a subroutine.

- Operand: none
- Stack effect: `( -- )`
- Pops the return address from the call stack into `ip`.
- Error: `RET` with an empty call stack halts the VM.

## Timing

### WAIT

Pauses the program.

- Operand: none
- Stack effect: `( ms -- )`
- Blocks the VM task with `vTaskDelay`, so other tasks and the idle task keep running.
- Resolution is one FreeRTOS tick (10 ms at `CONFIG_FREERTOS_HZ=100`); `ms` is rounded down to whole ticks.
- Negative `ms` is treated as `0`.

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

The VM does not return `Error` codes yet. On the conditions below it logs an error and halts (`running = false`):

| Condition            | Trigger                                                  |
|----------------------|----------------------------------------------------------|
| Bad opcode           | Opcode value outside the enum, or with no handler        |
| Bad local index      | `LOAD` / `STORE` with `idx` outside `0 .. VM_LOCALS - 1` |
| Call stack overflow  | `CALL` with `VM_CALL_DEPTH` calls already active         |
| Call stack underflow | `RET` with no active call                                |

Operand stack overflow/underflow and out-of-range jump/call targets are **not** checked, so `STACK_OVERFLOW` is never raised.
