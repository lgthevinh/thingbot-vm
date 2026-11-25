# Virtual machine Specifications

Stack-based VM that runs Thingcode programs on ThingBot (ESP32-C3). It is split in three parts: the **Engine** interprets the program, the **Runtime** is the board environment that natives act on, and the **Vm** ties them together and is what `main` uses. The core (`vm/`) has no hardware or FreeRTOS dependencies. Board-specific code lives in `main/native/`: the runtime and its natives, called with `INVOKE_NATIVE`, on top of the hardware layer in `main/native/board/`. Opcode semantics are specified in [opcode.md](opcode.md).

## Table of Contents

- [Architecture](#architecture)
  - [Vm](#vm)
  - [Engine](#engine)
  - [Runtime](#runtime)
  - [Word](#word)
  - [Thingcode program](#thingcode-program)
  - [Registers](#registers)
  - [Operand stack](#operand-stack)
  - [Globals](#globals)
  - [Call stack](#call-stack)
  - [Linear memory](#linear-memory)
- [Source layout](#source-layout)
- [Lifecycle](#lifecycle)
  - [vm_init](#vm_init)
  - [vm_load](#vm_load)
  - [vm_run](#vm_run)
- [Execution loop](#execution-loop)
- [Dispatch](#dispatch)
- [Natives](#natives)
  - [Id ranges](#id-ranges)
  - [Tables](#tables)
- [Memory](#memory)
  - [ESP32-C3](#esp32-c3)
  - [VM footprint](#vm-footprint)
- [Errors and halting](#errors-and-halting)
- [Limitations](#limitations)

## Architecture

```
vm_t ──owns──▶ prog, engine_t
  │
  ├──points to──▶ common, specific native tables
  └──points to──▶ runtime_t (board)

vm_run ──▶ engine_step(engine, prog, common, specific, runtime)
                └─ INVOKE_NATIVE id ──▶ fn(engine, runtime)
```

### Vm

`vm_t` (`vm/vm.h`) is the facade. It holds the program and the engine, points to the two native tables, and to the runtime. It exposes `vm_init`, `vm_load` and `vm_run`, and passes the runtime to the engine on every step.

```c
typedef struct {
    const int *prog;                // program being run
    const native_table_t *common;   // natives every runtime provides, ids below NATIVE_SPECIFIC_BASE
    const native_table_t *specific; // natives of this runtime only, ids from NATIVE_SPECIFIC_BASE
    engine_t engine;
    runtime_t *runtime;             // board environment, passed to every native
} vm_t;
```

The runtime is held by pointer: its contents are defined by the board, so `vm.h` cannot embed it without depending on the board.

### Engine

`engine_t` (`vm/engine/engine.h`) is the interpreter only. It holds the registers, the operand stack, the call stack, the globals and linear memory, and executes one instruction per `engine_step`. It exposes `engine_push` and `engine_pop` so natives can reach the operand stack. It knows nothing about the board.

```c
struct engine_t {
    int pc;                        // index of the next slot to fetch
    int sp;                        // index of the operand stack top, -1 when empty
    int stack[VM_STACK_LEN];       // operand stack
    int call_sp;                   // index of the call stack top, -1 when empty
    int call_stack[VM_CALL_DEPTH]; // return addresses
    int global[VM_GLOBAL_SLOT];    // global slots
    uint8_t mem[VM_MEM_LEN];       // linear memory
    EngineStatus status;           // halted, running or waiting
    int wait_ms;                   // pause requested by the last WAIT
};
```

| Constant         | Value | Meaning              |
|------------------|------:|----------------------|
| `VM_STACK_LEN`   | 256   | Operand stack slots  |
| `VM_GLOBAL_SLOT` | 256   | Global slots, 1 KB   |
| `VM_CALL_DEPTH`  | 64    | Maximum nested calls |
| `VM_MEM_LEN`     | 4096  | Linear memory bytes  |

| `EngineStatus`   | Meaning                                                         |
|------------------|-----------------------------------------------------------------|
| `ENGINE_HALTED`  | Stopped by `HLT` or an error. `0`, so a zeroed engine is halted |
| `ENGINE_RUNNING` | Executing instructions                                          |
| `ENGINE_WAITING` | Paused by `WAIT` for `wait_ms`, resumed by the next `vm_run`    |

### Runtime

`runtime_t` is the board environment that natives act on, and the context passed to every native. The core only declares the name, together with `NativeFn`, `native_table_t` and the id ranges, in `vm/runtime/runtime.h`, and never reads it. The board defines its contents. On ThingBot (`native/native.h`):

```c
struct runtime_t {
    io_t io;          // I2C bus, SPI host, GPIO pin masks
    act_handle_t act; // PCA9685
};
```

`main` creates the buses, initializes `io` and `act`, and passes `&runtime` to `vm_init`. The VM never creates or frees hardware resources.

### Word

The only data type is the C `int`, which is 32-bit signed on the ESP32-C3. Program slots, stack entries, globals and return addresses are all `int`. Comparisons produce `1` or `0`, and `JNZ` treats any non-zero value as true.

### Thingcode program

A Thingcode program is a `const int` array, addressed by slot index. Each opcode takes one slot, and its operand, if it has one, takes the next slot. There is no header and no length, so a program ends where its `HLT` is executed. Jump and call targets are slot indices.

```c
const int prog[] = {
    PSH, 2,   // slot 0-1
    PSH, 3,   // slot 2-3
    ADD,      // slot 4
    HLT,      // slot 5
};
```

### Registers

Held by the engine.

| Register  | Holds                                                                |
|-----------|----------------------------------------------------------------------|
| `pc`      | Index of the next code slot to fetch                                 |
| `sp`      | Index of the operand stack top, `-1` when empty                      |
| `call_sp` | Index of the call stack top, `-1` when empty                         |
| `status`  | `vm_run` steps while `ENGINE_RUNNING`, set by `HLT`, `WAIT` or error |
| `wait_ms` | Pause requested by the last `WAIT`, read by the caller of `vm_run`   |

### Operand stack

`stack` holds the values that opcodes work on. It grows upward: a push writes `stack[++sp]` and a pop reads `stack[sp--]`. Arithmetic, comparison opcodes and natives pop their inputs and push their result. Bounds are not checked (see [Limitations](#limitations)).

### Globals

`global` is a single flat array of 256 slots (1 KB), accessed by index with `GLOAD` and `GSTORE`. The index is bounds-checked.

- **Global to the program:** there are no per-call frames, so a callee writing `global[idx]` overwrites the caller's value.
- **Cleared on load:** `vm_load` sets every slot to `0`.

### Call stack

`call_stack` holds return addresses only: no arguments, globals or saved `sp`. `CALL` pushes the index of the slot after its operand and jumps; `RET` pops it back into `pc`. Overflow and underflow are checked. Arguments and return values pass through the operand stack.


### Linear memory

`mem` is a byte array of `VM_MEM_LEN` (4096) bytes, the WASM model: flat, with no paging or address translation. Every access is bounds-checked, so a program cannot reach anything outside it.

- Programs access it with `MLOAD8`, `MSTORE8`, `MLOAD32` and `MSTORE32` (see [opcode.md](opcode.md#memory)). Constant data, such as names and UUIDs, is written with the same opcodes.
- Natives get buffers from it as `ptr len` pairs, through `engine_mem`:

  ```c
  uint8_t *engine_mem(engine_t *e, int ptr, int len);
  ```

  It returns a pointer to `mem[ptr]` when `ptr .. ptr + len - 1` fits in memory and `len >= 0`. Otherwise it logs `bad memory access`, halts the engine and returns `NULL`.
- Words are 32-bit little-endian, and addresses need no alignment.
- **Cleared on load:** `vm_load` sets every byte to `0`, so a program never sees the previous program's buffers.
## Source layout

```
main/
  main.c
  vm/
    vm.h  vm.c
    engine/    engine.h engine.c  handler.h handler.c  opcode.h  error.h
    runtime/   runtime.h
  native/
    native.h  native.c
    board/     io.h io.c  act.h act.c
```

| Module                   | Role                                                              |
|--------------------------|-------------------------------------------------------------------|
| `vm/vm.h/.c`             | Vm facade: `vm_init`, `vm_load`, `vm_run`                         |
| `vm/engine/engine.h/.c`  | Engine state, fetch/push/pop, `engine_step`                       |
| `vm/engine/handler.h/.c` | One handler per opcode and the `op_handlers` dispatch table       |
| `vm/engine/opcode.h`     | `OpCode` enum, `OP_COUNT`                                         |
| `vm/engine/error.h`      | `Error` enum (not used yet)                                       |
| `vm/runtime/runtime.h`   | `runtime_t` name, `NativeFn`, `native_table_t`, native id ranges  |
| `native/native.h/.c`     | ThingBot `runtime_t` contents, natives, `NativeId`, native tables |
| `native/board/io.h/.c`   | I2C bus, SPI host and GPIO pin masks                              |
| `native/board/act.h/.c`  | Actuators on the PCA9685                                          |

Nothing in `vm/` includes a `native/`, ESP-IDF driver or FreeRTOS header; the dependency runs one way, from `native/` to `runtime.h` and `engine.h`. `engine.c`, `handler.c` and `native.c` use `esp_log.h` for logging.

## Lifecycle

```c
static runtime_t runtime;
static vm_t vm;

vm_init(&vm, &runtime, &thingbot_common, &thingbot_specific);
vm_load(&vm, prog);

while (vm_run(&vm) == ENGINE_WAITING) {
    vTaskDelay(pdMS_TO_TICKS(vm.engine.wait_ms));
}
```

### vm_init

`void vm_init(vm_t *vm, runtime_t *runtime, const native_table_t *common, const native_table_t *specific)`

Stores the runtime and its two native tables (see [Natives](#natives)). Called once before the first load.

### vm_load

`void vm_load(vm_t *vm, const int *prog)`

Sets `prog` and resets the engine to a fresh start:

- `pc = 0`, `sp = -1`, `call_sp = -1`, `status = ENGINE_RUNNING`
- every slot of `global` and every byte of `mem` is set to `0`
- the runtime and the native tables are kept.

The same `vm_t` can load and run several programs one after another.

### vm_run

`EngineStatus vm_run(vm_t *vm)`

Steps the engine until it halts or waits, and returns the status:

- `ENGINE_HALTED`: the program ended with `HLT` or an error. Calling `vm_run` again returns `ENGINE_HALTED` at once.
- `ENGINE_WAITING`: the program executed `WAIT`. The caller pauses for `vm.engine.wait_ms`, then calls `vm_run` again to resume after the `WAIT`.

The engine never sleeps itself, so how to pause is up to the caller. On ThingBot, `main` blocks the task with `vTaskDelay`, so the idle task and watchdog keep running.

## Execution loop

```
vm_run:
    while status == RUNNING:
        engine_step:
            opcode = prog[pc++]
            dispatch(opcode)
```

- Handlers that take an operand fetch it with `engine_fetch`, which also advances `pc`.
- After a handler returns, `pc` points at the next instruction unless the handler changed it (`JMP`, `JNZ`, `CALL`, `RET`).
- Log lines report `pc` as the index of the instruction being executed: `pc - 1` for opcodes without an operand, `pc - 2` for opcodes with one.

## Dispatch

`engine_step` in `engine.c` dispatches each opcode:

- `INVOKE_NATIVE` is handled by `engine_step` itself, since it is the only opcode that needs the native tables and the runtime (see [Natives](#natives)).
- Every other opcode goes through `op_handlers` in `handler.c`, a table of `OpHandler` (`void (*)(engine_t *, const int *prog)`) indexed by opcode and sized `OP_COUNT`.

Before calling a handler, `engine_step` checks that:

- `0 <= opcode < OP_COUNT`
- `op_handlers[opcode]` is not `NULL`

Otherwise it logs `bad opcode` and halts. An opcode declared in `opcode.h` without a table entry fails this check.

Adding an opcode:

1. Add it to `OpCode` before `OP_COUNT`.
2. Write `static void op_<name>(engine_t *e, const int *prog)` in `handler.c`.
3. Add `[<NAME>] = op_<name>` to `op_handlers`.
4. Document it in [opcode.md](opcode.md).

Board functions are added as natives instead (see [Natives](#natives)).

## Natives

A native is a runtime function that receives the engine and the runtime:

```c
typedef void (*NativeFn)(engine_t *e, runtime_t *rt);
```

A native is a runtime function that receives the engine and the runtime. It pops its own arguments with `engine_pop`, reaches buffers through `engine_mem`, reaches hardware through `rt`, and halts by setting `e->status = ENGINE_HALTED` on error.

### Id ranges

Native ids are 16-bit. `0x0000 .. 0x7FFF` are common natives: core (`0x0000 .. 0x0FFF`) on every runtime, and optional modules such as connectivity (`0x1000 .. 0x1FFF`). `0x8000 .. 0xFFFF` are specific to one runtime. The ranges, modules and common natives are specified in [native_common.md](native_common.md).

### Tables

A runtime registers one table per range with `vm_init`:

```c
typedef struct {
    const NativeFn *fns; // fns[i] is id base + i
    int count;
} native_table_t;
```

Each table is dense from its base: `common->fns[id]` and `specific->fns[id - NATIVE_SPECIFIC_BASE]`. A table with no natives is `{0}`.

`INVOKE_NATIVE id` picks the table by bit 15, then checks that the index is below `count` and the entry is not `NULL`, and calls `fn(engine, runtime)`. An `id` outside `0 .. 0xFFFF`, past the end of its table, or with no entry logs `bad native id` and halts.

On ThingBot the tables are `thingbot_common` (empty) and `thingbot_specific`, in `native/native.c`. Ids are defined by `NativeId` in `native/native.h` and listed in [native_thingbot.md](native_thingbot.md).

Adding a specific native:

1. Add it to `NativeId` before `NATIVE_END`.
2. Write `static void native_<name>(engine_t *e, runtime_t *rt)` in `native.c`.
3. Add `[<NAME> - NATIVE_SPECIFIC_BASE] = native_<name>` to `specific_fns`.
4. Document it in [native_thingbot.md](native_thingbot.md).

Common natives are added for all runtimes at once (see [native_common.md](native_common.md#adding-a-common-native)).

## Memory

### ESP32-C3

Single-core 32-bit RISC-V at up to 160 MHz, with 400 KB of on-chip SRAM (16 KB of it reserved for the flash cache) and 8 KB of RTC SRAM. Code and read-only data run from external flash (`CONFIG_ESPTOOLPY_FLASHSIZE` is 2 MB).

On the C3, instruction RAM and data RAM share the same SRAM, so RAM-resident code, static data and the heap all come out of one pool. Current build (`idf.py size`):

| Region                     | Size      |
|----------------------------|----------:|
| Usable DRAM                | 321,296 B |
| IRAM code (`.text`)        | 41,406 B  |
| Initialized data (`.data`) | 6,925 B   |
| Zeroed data (`.bss`)       | 5,224 B   |
| Left for heap              | ~267 KB   |

Statics such as `vm` and `runtime` go in `.bss`. FreeRTOS task stacks are allocated from the heap when each task is created:

| Task       | Stack  | Config                                   |
|------------|-------:|------------------------------------------|
| `app_main` | 3584 B | `CONFIG_ESP_MAIN_TASK_STACK_SIZE`        |
| Timer      | 2048 B | `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH` |
| Idle       | 1536 B | `CONFIG_FREERTOS_IDLE_TASK_STACKSIZE`    |

Stack overflow checking is off (`CONFIG_STACK_CHECK_NONE`). To measure at runtime, use `esp_get_free_heap_size()` for the heap and `uxTaskGetStackHighWaterMark(NULL)` for the calling task's unused stack.

### VM footprint

`vm_t`, including its `engine_t`, is about 6.4 KB on the ESP32-C3:

| Part         | Size   |
|--------------|-------:|
| `mem`        | 4096 B |
| `stack`      | 1024 B |
| `global`     | 1024 B |
| `call_stack` | 256 B  |
| Other fields | ~36 B  |

The `app_main` task stack is 3584 B (`CONFIG_ESP_MAIN_TASK_STACK_SIZE`), so `vm_t` and `runtime_t` are declared `static` in `main.c`.

## Errors and halting

The engine stops when `status` becomes `ENGINE_HALTED`. It halts normally on `HLT`, and on these errors it logs with `ESP_LOGE` and halts:

| Condition            | Raised by                               |
|----------------------|-----------------------------------------|
| Bad opcode           | `engine_step`                           |
| Bad global index     | `GLOAD`, `GSTORE`                       |
| Division by zero     | `DIV`, `MOD`                            |
| Call stack overflow  | `CALL`                                  |
| Call stack underflow | `RET`                                   |
| Not implemented      | `INVOKE_STATIC`                         |
| Bad native id        | `INVOKE_NATIVE`                         |
| Bad memory access    | `MLOAD*`, `MSTORE*`, `engine_mem`       |

`vm_run` returns `ENGINE_HALTED` in both cases, so the caller cannot tell a normal halt from an error. The `Error` enum in `error.h` is reserved for this and is not used yet.

## Limitations

These are not checked and lead to undefined behaviour:

- **Operand stack overflow/underflow:** pushing more than `VM_STACK_LEN` values, or popping from an empty stack.
- **Running past the end of the program:** a path that does not end with `HLT`.
- **Out-of-range jump/call targets:** `JMP`, `JNZ` and `CALL` set `pc` to any value.

Other behaviour to be aware of:

- **Integer overflow:** `ADD`, `SUB` and `MUL` are plain C `int` with no overflow handling.
- **Globals:** shared with calls (see [Globals](#globals)).
