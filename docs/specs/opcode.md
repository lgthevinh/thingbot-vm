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
- [Globals](#globals)
  - [GLOAD](#gload), [GSTORE](#gstore)
- [Branching](#branching)
  - [JMP](#jmp), [JNZ](#jnz)
- [Calls](#calls)
  - [CALL](#call), [RET](#ret), [INVOKE_STATIC](#invoke_static), [INVOKE_NATIVE](#invoke_native)
- [Timing](#timing)
  - [WAIT](#wait)
- [Memory](#memory)
  - [MLOAD8](#mload8), [MSTORE8](#mstore8), [MLOAD32](#mload32), [MSTORE32](#mstore32)
- [Errors](#errors)
  - [Error codes](#error-codes)
  - [Current behavior](#current-behavior)

## Conventions

- A Thingcode program is an array of `int`: a header, a data section, then the code (see [vm.md](vm.md#thingcode-program)). In the code, each opcode occupies one slot; its operand (if any) occupies the next slot. `pc` and jump targets index the code, not the whole array.
- Stack effect notation: `( before -- after )`, top of stack on the right.
- For binary operators, `a` is the value pushed first and `b` is the value pushed last (top of stack).
- Comparison results are `1` (true) or `0` (false).
- `VM_GLOBAL_SLOT` is the number of global slots; valid global indices are `0 .. VM_GLOBAL_SLOT - 1`.
- `VM_CALL_DEPTH` is the maximum number of nested calls.
- `VM_MEM_LEN` is the size of linear memory in bytes; valid addresses are `0 .. VM_MEM_LEN - 1`.
- New opcodes are only appended: an existing value never changes, so compiled programs stay valid.

## Opcode Summary

| Value | Mnemonic        | Category   | Operand     | Stack effect         |
|------:|-----------------|------------|-------------|----------------------|
| 0     | `HLT`           | Control    | -           | `( -- )`             |
| 1     | `PSH`           | Stack      | `value`     | `( -- value )`       |
| 2     | `ADD`           | Arithmetic | -           | `( a b -- a+b )`     |
| 3     | `SUB`           | Arithmetic | -           | `( a b -- a-b )`     |
| 4     | `MUL`           | Arithmetic | -           | `( a b -- a*b )`     |
| 5     | `DIV`           | Arithmetic | -           | `( a b -- a/b )`     |
| 6     | `MOD`           | Arithmetic | -           | `( a b -- a%b )`     |
| 7     | `GLOAD`         | Globals    | `idx`       | `( -- global[idx] )` |
| 8     | `GSTORE`        | Globals    | `idx`       | `( v -- )`           |
| 9     | `JMP`           | Branching  | `target_pc` | `( -- )`             |
| 10    | `JNZ`           | Branching  | `target_pc` | `( cond -- )`        |
| 11    | `GT`            | Comparison | -           | `( a b -- a>b )`     |
| 12    | `GTE`           | Comparison | -           | `( a b -- a>=b )`    |
| 13    | `LT`            | Comparison | -           | `( a b -- a<b )`     |
| 14    | `LTE`           | Comparison | -           | `( a b -- a<=b )`    |
| 15    | `EQ`            | Comparison | -           | `( a b -- a==b )`    |
| 16    | `NEQ`           | Comparison | -           | `( a b -- a!=b )`    |
| 17    | `CALL`          | Calls      | `target_pc` | `( -- )`             |
| 18    | `RET`           | Calls      | -           | `( -- )`             |
| 19    | `WAIT`          | Timing     | -           | `( ms -- )`          |
| 20    | `INVOKE_STATIC` | Calls      | -           | -                    |
| 21    | `INVOKE_NATIVE` | Calls      | `id`        | `( args -- )`        |
| 22    | `MLOAD8`        | Memory     | -           | `( addr -- byte )`   |
| 23    | `MSTORE8`       | Memory     | -           | `( addr byte -- )`   |
| 24    | `MLOAD32`       | Memory     | -           | `( addr -- word )`   |
| 25    | `MSTORE32`      | Memory     | -           | `( addr word -- )`   |

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

## Globals

### GLOAD

Pushes the value of a global slot onto the stack.

- Operand: `idx`
- Stack effect: `( -- global[idx] )`
- Error: `idx` out of range halts the VM.

### GSTORE

Pops the top of the stack into a global slot.

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

Return addresses are kept on a separate call stack, up to `VM_CALL_DEPTH` deep. The operand stack and globals are shared between caller and callee: values pushed before `CALL` are visible to the callee, values left on the stack before `RET` are visible to the caller, and a callee writing `global[idx]` overwrites the caller's value.

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

- Operand: `id`, a 16-bit native id. `0x0000 .. 0x7FFF` are common natives, the same on every runtime that provides them; `0x8000 .. 0xFFFF` are specific to one runtime (see [native_common.md](native_common.md#id-ranges)).
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

## Memory

Linear memory is a byte array of `VM_MEM_LEN` (4096) bytes in the engine. `vm_load` clears it to zero and copies the program's data section to the start (see [vm.md](vm.md#linear-memory)). Constant data such as names and UUIDs goes in the data section, not in the code. Natives read and write buffers in it as `ptr len` pairs.

- Words are 32-bit little-endian: the low byte is at the lowest address.
- Addresses need no alignment.
- Every access is bounds-checked. An access that does not fit in `0 .. VM_MEM_LEN - 1` halts the VM.

### MLOAD8

Pushes one byte from memory.

- Operand: none
- Stack effect: `( addr -- byte )`
- `byte` is `0 .. 255`.
- Error: `addr` outside `0 .. VM_MEM_LEN - 1` halts the VM.

### MSTORE8

Stores one byte into memory.

- Operand: none
- Stack effect: `( addr byte -- )`
- Stores the low 8 bits of `byte`; the higher bits are ignored.
- Error: `addr` outside `0 .. VM_MEM_LEN - 1` halts the VM.

### MLOAD32

Pushes one word from memory.

- Operand: none
- Stack effect: `( addr -- word )`
- Reads the 4 bytes `addr .. addr + 3` as a signed 32-bit word.
- Error: `addr` outside `0 .. VM_MEM_LEN - 4` halts the VM.

### MSTORE32

Stores one word into memory.

- Operand: none
- Stack effect: `( addr word -- )`
- Writes the 4 bytes `addr .. addr + 3`.
- Error: `addr` outside `0 .. VM_MEM_LEN - 4` halts the VM.

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

The VM does not return `Error` codes yet. On the conditions below it logs an error and halts (`status = ENGINE_HALTED`):

| Condition            | Trigger                                                         |
|----------------------|-----------------------------------------------------------------|
| Bad opcode           | Opcode value outside the enum, or with no handler               |
| Bad global index     | `GLOAD` / `GSTORE` with `idx` outside `0 .. VM_GLOBAL_SLOT - 1` |
| Division by zero     | `DIV` / `MOD` with `b == 0`                                     |
| Call stack overflow  | `CALL` with `VM_CALL_DEPTH` calls already active                |
| Call stack underflow | `RET` with no active call                                       |
| Not implemented      | `INVOKE_STATIC`                                                 |
| Bad native id        | `INVOKE_NATIVE` with `id` not in the native tables              |
| Bad memory access    | `MLOAD*` / `MSTORE*` outside `0 .. VM_MEM_LEN - 1`              |
| Pc out of range      | Fetching past the end of the code                               |

Operand stack overflow/underflow and out-of-range jump/call targets are **not** checked, so `STACK_OVERFLOW` is never raised.
