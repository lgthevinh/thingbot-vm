# Common Native Specification

Natives that every runtime provides, called with [`INVOKE_NATIVE id`](opcode.md#invoke_native). A program that uses only common natives runs on any runtime. Runtime-specific natives are specified per runtime, e.g. [native_thingbot.md](native_thingbot.md) for ThingBot. How natives are registered and looked up is described in [vm.md](vm.md#natives).

## Table of Contents

- [Id ranges](#id-ranges)
- [Native Summary](#native-summary)
- [Adding a common native](#adding-a-common-native)

## Id ranges

Native ids are 16-bit, `0x0000 .. 0xFFFF`, split in two halves by bit 15:

| Range              | Kind     | Meaning                                                                             |
|--------------------|----------|-------------------------------------------------------------------------------------|
| `0x0000 .. 0x7FFF` | Common   | Same id and behaviour on every runtime, so a program using only these runs anywhere |
| `0x8000 .. 0xFFFF` | Specific | Defined by one runtime, a program using them needs that runtime                     |

`NATIVE_SPECIFIC_BASE` (`0x8000`) and `NATIVE_ID_MAX` (`0xFFFF`) are defined in `main/vm/runtime/runtime.h`.

- Every runtime implements the common natives its own way, with the stack effect and behaviour given here.
- Specific ids are not unique across runtimes: `0x8000` on one runtime and `0x8000` on another can be different functions. Nothing checks which runtime a program was written for yet.

## Native Summary

None yet. ThingBot registers an empty common table (`thingbot_common`).

| Id | Name | Stack effect | Behaviour |
|---:|------|--------------|-----------|

## Adding a common native

1. Agree on its id, stack effect and behaviour for all runtimes, and list it in [Native Summary](#native-summary). Ids are assigned from `0x0000` upward with no gaps, since each runtime's common table is dense.
2. Implement it in every runtime, at index `id` of that runtime's common table. On ThingBot, add it to `thingbot_common` in `main/native/native.c`.
