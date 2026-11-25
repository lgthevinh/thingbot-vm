# Common Native Specification

Natives with the same id and behaviour on every runtime, called with [`INVOKE_NATIVE id`](opcode.md#invoke_native). Runtime-specific natives are specified per runtime, e.g. [native_thingbot.md](native_thingbot.md) for ThingBot. How natives are registered and looked up is described in [vm.md](vm.md#natives).

## Table of Contents

- [Id ranges](#id-ranges)
- [Modules](#modules)
- [Module registry](#module-registry)
- [Core](#core)
- [Adding a common native](#adding-a-common-native)

## Id ranges

Native ids are 16-bit, `0x0000 .. 0xFFFF`, split in two halves by bit 15:

| Range              | Kind     | Meaning                                                         |
|--------------------|----------|-----------------------------------------------------------------|
| `0x0000 .. 0x7FFF` | Common   | Same id and behaviour on every runtime that provides it         |
| `0x8000 .. 0xFFFF` | Specific | Defined by one runtime, a program using them needs that runtime |

The common half is divided by category:

| Range              | Category     | Modules        | Provided by                          |
|--------------------|--------------|----------------|--------------------------------------|
| `0x0000 .. 0x0FFF` | Core         | `0x00 .. 0x0F` | Every runtime                        |
| `0x1000 .. 0x1FFF` | Connectivity | `0x10 .. 0x1F` | Runtimes with the hardware, optional |
| `0x2000 .. 0x7FFF` | Unassigned   | `0x20 .. 0x7F` | -                                    |

`NATIVE_SPECIFIC_BASE` (`0x8000`) and `NATIVE_ID_MAX` (`0xFFFF`) are defined in `main/vm/runtime/runtime.h`.

- A program that uses only core natives runs on any runtime.
- Specific ids are not unique across runtimes: `0x8000` on one runtime and `0x8000` on another can be different functions. Nothing checks which runtime a program was written for yet.

## Modules

A module is a group of related natives that takes one 256-id block. Its id is the high byte of its native ids:

```
native id = module id << 8 | index
```

- A runtime provides a module completely or not at all. Calling a native of a module the runtime does not provide halts with `bad native id`.
- Each module defines what its natives push. The result convention of the connectivity modules applies to those modules only.
- Modules only grow: an existing id never changes its number or behaviour, and new natives take the next free index. A breaking change gets a new module id.

The current lookup indexes the common table directly by id, so registering a module at `0x1000` would need a table of at least `0x1000` entries. Lookup by module id is needed before a runtime registers a connectivity module.

## Module registry

| Module | Ids                | Name  | Category     | Spec                                                 |
|-------:|--------------------|-------|--------------|------------------------------------------------------|
| `0x10` | `0x1000 .. 0x10FF` | `TCP` | Connectivity | [native_connectivity.md](native_connectivity.md#tcp) |
| `0x11` | `0x1100 .. 0x11FF` | `UDP` | Connectivity | [native_connectivity.md](native_connectivity.md#udp) |
| `0x12` | `0x1200 .. 0x12FF` | `BLE` | Connectivity | [native_connectivity.md](native_connectivity.md#ble) |

## Core

No core natives yet. ThingBot registers an empty common table (`thingbot_common`).

## Adding a common native

1. Agree on its id, stack effect and behaviour for all runtimes. Add it to its module's spec at the next free index, or register a new module in [Module registry](#module-registry).
2. Implement it in every runtime that provides the module. On ThingBot, add it to `thingbot_common` in `main/native/native.c`.
