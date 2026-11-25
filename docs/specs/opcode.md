# Thingcode Opcode Specification

Thingcode is the bytecode run by the stack-based VM. Opcodes are defined in `main/vm/engine/opcode.h` and dispatched by `engine_step` in `main/vm/engine/engine.c` to the handlers in `main/vm/engine/handler.c`. Error codes are defined in `main/vm/engine/error.h`. Runtime functions reached through `INVOKE_NATIVE` are specified in [native_common.md](native_common.md) and, for ThingBot, [native_thingbot.md](native_thingbot.md).

## Table of Contents

- [Conventions](#conventions)
- [Opcode Summary](#opcode-summary)
- [Control](#control)
  - [HLT](#hlt)
- [Stack](#stack)
  - [PSH](#psh)
- [Arithmetic](#arithmetic)
  - [ADD](#add), [SUB](#sub), [MUL](#mul), [DIV](#div), [MOD](#mod)
- [Comparison](#comparison)
  - [GT](#gt), [GTE](#gte), [LT](#lt), [LTE](#lte), [EQ](#eq), [NEQ](#neq)
- [Locals](#locals)
  - [LOAD](#load), [STORE](#store)
- [Branching](#branching)
  - [JMP](#jmp), [JNZ](#jnz)
- [Calls](#calls)
  - [CALL](#call), [RET](#ret), [INVOKE_STATIC](#invoke_static), [INVOKE_NATIVE](#invoke_native)
- [Timing](#timing)
  - [WAIT](#wait)
- [Errors](#errors)
  - [Error codes](#error-codes)
  - [Current behavior](#current-behavior)

## Conventions

- A Thingcode program is an array of `int`. Each opcode occupies one slot; its operand (if any) occupies the next slot.
- Stack effect notation: `( before -- after )`, top of stack on the right.
- For binary operators, `a` is the value pushed first and `b` is the value pushed last (top of stack).
- Comparison results are `1` (true) or `0` (false).
- `VM_LOCAL_SLOT` is the number of local slots; valid local indices are `0 .. VM_LOCAL_SLOT - 1`.
- `VM_CALL_DEPTH` is the maximum number of nested calls.

## Opcode Summary

| Value | Mnemonic        | Category   | Operand     | Stack effect        |
|------:|-----------------|------------|-------------|---------------------|
| 0     | `HLT`           | Control    | -           | `( -- )`            |
| 1     | `PSH`           | Stack      | `value`     | `( -- value )`      |
| 2     | `ADD`           | Arithmetic | -           | `( a b -- a+b )`    |
| 3     | `SUB`           | Arithmetic | -           | `( a b -- a-b )`    |
| 4     | `MUL`           | Arithmetic | -           | `( a b -- a*b )`    |
| 5     | `DIV`           | Arithmetic | -           | `( a b -- a/b )`    |
| 6     | `MOD`           | Arithmetic | -           | `( a b -- a%b )`    |
| 7     | `LOAD`          | Locals     | `idx`       | `( -- local[idx] )` |
| 8     | `STORE`         | Locals     | `idx`       | `( v -- )`          |
| 9     | `JMP`           | Branching  | `target_pc` | `( -- )`            |
| 10    | `JNZ`           | Branching  | `target_pc` | `( cond -- )`       |
| 11    | `GT`            | Comparison | -           | `( a b -- a>b )`    |
| 12    | `GTE`           | Comparison | -           | `( a b -- a>=b )`   |
| 13    | `LT`            | Comparison | -           | `( a b -- a<b )`    |
| 14    | `LTE`           | Comparison | -           | `( a b -- a<=b )`   |
| 15    | `EQ`            | Comparison | -           | `( a b -- a==b )`   |
| 16    | `NEQ`           | Comparison | -           | `( a b -- a!=b )`   |
| 17    | `CALL`          | Calls      | `target_pc` | `( -- )`            |
| 18    | `RET`           | Calls      | -           | `( -- )`            |
| 19    | `WAIT`          | Timing     | -           | `( ms -- )`         |
| 20    | `INVOKE_STATIC` | Calls      | -           | -                   |
| 21    | `INVOKE_NATIVE` | Calls      | `id`        | `( args -- )`       |

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

Pop `b`, then `a`, push the result. Integer arithmetic; `ADD`, `SUB` and `MUL` have no overflow handling.

### ADD

- Stack effect: `( a b -- a+b )`

### SUB

- Stack effect: `( a b -- a-b )`

### MUL

- Stack effect: `( a b -- a*b )`

### DIV

- Stack effect: `( a b -- a/b )`
- Truncates toward zero: `-7 / 2` is `-3`.
- `INT_MIN / -1` wraps to `INT_MIN`.
- Error: `b == 0` halts the VM.

### MOD

- Stack effect: `( a b -- a%b )`
- Remainder of the truncating division, with the sign of `a`: `-7 % 2` is `-1`.
- `INT_MIN % -1` is `0`.
- Error: `b == 0` halts the VM.

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

- Operand: `target_pc`
- Stack effect: `( -- )`
- Sets `pc = target_pc`.

### JNZ

Conditional jump. The condition is always popped, so loops do not grow the stack.

- Operand: `target_pc`
- Stack effect: `( cond -- )`
- Sets `pc = target_pc` if `cond != 0`; otherwise continues with the next instruction.

## Calls

Return addresses are kept on a separate call stack, up to `VM_CALL_DEPTH` deep. The operand stack and locals are shared between caller and callee: values pushed before `CALL` are visible to the callee, values left on the stack before `RET` are visible to the caller, and a callee writing `local[idx]` overwrites the caller's value.

### CALL

Calls a subroutine.

- Operand: `target_pc`
- Stack effect: `( -- )`
- Pushes the address of the next instruction onto the call stack, then sets `pc = target_pc`.
- Error: calling when the call stack already holds `VM_CALL_DEPTH` entries halts the VM.

### RET

Returns from a subroutine.

- Operand: none
- Stack effect: `( -- )`
- Pops the return address from the call stack into `pc`.
- Error: `RET` with an empty call stack halts the VM.

### INVOKE_STATIC

Reserved, not implemented yet.

- Error: executing `INVOKE_STATIC` logs `invoke static not implemented` and halts the VM.

### INVOKE_NATIVE

Calls a runtime function from the native tables registered with `vm_init`.

- Operand: `id`, a 16-bit native id. `0x0000 .. 0x7FFF` are common natives, available on every runtime; `0x8000 .. 0xFFFF` are specific to one runtime (see [native_common.md](native_common.md#id-ranges)).
- Stack effect: `( args -- )`, depends on the native; each native pops its own arguments
- Does not touch the call stack.
- Error: `id` outside `0 .. 0xFFFF`, past the end of its table, or with no entry halts the VM.
- Errors raised by the native itself are listed in [native_thingbot.md](native_thingbot.md#errors) for ThingBot.

## Timing

### WAIT

Pauses the program.

- Operand: none
- Stack effect: `( ms -- )`
- Sets the engine to `ENGINE_WAITING` with `wait_ms = ms`, and `vm_run` returns. The engine does not sleep itself: the caller pauses, then calls `vm_run` again to continue after the `WAIT` (see [vm.md](vm.md#vm_run)).
- On ThingBot, `main` pauses with `vTaskDelay`, so other tasks and the idle task keep running. Resolution is one FreeRTOS tick (10 ms at `CONFIG_FREERTOS_HZ=100`); `ms` is rounded down to whole ticks.
- Negative `ms` is treated as `0`.

## Errors

### Error codes

Defined by the `Error` enum in `error.h`.

| Value | Name             | Meaning                                      |
|------:|------------------|
| 0     | `SUCCESS`        | No error (0 is reserved for success)         |
| 1     | `INTERNAL`       | Internal implementation error of the VM      |
| 2     | `STACK_OVERFLOW` | Stack overflow                               |
| 3     | `OUT_OF_MEM`     | Out of memory                                |
| 4     | `UNKNOWN`        | Unknown error                                |

### Current behavior

The VM does not return `Error` codes yet. On the conditions below it logs an error and halts (`status = ENGINE_HALTED`):

| Condition            | Trigger                                                      |
|----------------------|--------------------------------------------------------------|
| Bad opcode           | Opcode value outside the enum, or with no handler            |
| Bad local index      | `LOAD` / `STORE` with `idx` outside `0 .. VM_LOCAL_SLOT - 1` |
| Division by zero     | `DIV` / `MOD` with `b == 0`                                  |
| Call stack overflow  | `CALL` with `VM_CALL_DEPTH` calls already active             |
| Call stack underflow | `RET` with no active call                                    |
| Not implemented      | `INVOKE_STATIC`                                              |
| Bad native id        | `INVOKE_NATIVE` with `id` not in the native tables           |

Operand stack overflow/underflow and out-of-range jump/call targets are **not** checked, so `STACK_OVERFLOW` is never raised.
