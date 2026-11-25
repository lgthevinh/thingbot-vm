# Virtual machine Specifications

Stack-based VM that runs Thingcode programs on ThingBot (ESP32-C3). It is split in three parts: the **Engine** interprets the program, the **Runtime** is the board environment that natives act on, and the **Vm** ties them together and is what `main` uses. The core (`vm/`) has no hardware or FreeRTOS dependencies. Board-specific code lives in `main/board/` as the runtime and its natives, called with `INVOKE_NATIVE`. Opcode semantics are specified in [opcode_common.md](opcode_common.md).

## Table of Contents

- [Architecture](#architecture)
  - [Vm](#vm)
  - [Engine](#engine)
  - [Runtime](#runtime)
  - [Word](#word)
  - [Thingcode program](#thingcode-program)
  - [Registers](#registers)
  - [Operand stack](#operand-stack)
  - [Locals](#locals)
  - [Call stack](#call-stack)
- [Source layout](#source-layout)
- [Lifecycle](#lifecycle)
  - [vm_init](#vm_init)
  - [vm_load](#vm_load)
  - [vm_run](#vm_run)
- [Execution loop](#execution-loop)
- [Dispatch](#dispatch)
- [Natives](#natives)
- [Memory](#memory)
  - [ESP32-C3](#esp32-c3)
  - [VM footprint](#vm-footprint)
- [Errors and halting](#errors-and-halting)
- [Limitations](#limitations)

## Architecture

```
vm_t ──owns──▶ prog, natives, engine_t
  │
  └──points to──▶ runtime_t (board)

vm_run ──▶ engine_step(engine, prog, natives, runtime)
                └─ INVOKE_NATIVE ──▶ natives[id](engine, runtime)
```

### Vm

`vm_t` (`vm/vm.h`) is the facade. It holds the program, the native table and the engine, and points to the runtime. It exposes `vm_init`, `vm_load` and `vm_run`, and passes the runtime to the engine on every step.

```c
typedef struct {
    const int *prog;         // program being run
    const NativeFn *natives; // runtime functions, indexed by INVOKE_NATIVE id
    int native_count;        // number of entries in natives
    engine_t engine;
    runtime_t *runtime;      // board environment, passed to every native
} vm_t;
```

The runtime is held by pointer: its contents are defined by the board, so `vm.h` cannot embed it without depending on the board.

### Engine

`engine_t` (`vm/engine.h`) is the interpreter only. It holds the registers, the operand stack, the call stack and the locals, and executes one instruction per `engine_step`. It exposes `engine_push` and `engine_pop` so natives can reach the operand stack. It knows nothing about the board.

```c
struct engine_t {
    int pc;                        // index of the next slot to fetch
    int sp;                        // index of the operand stack top, -1 when empty
    int stack[VM_STACK_LEN];       // operand stack
    int call_sp;                   // index of the call stack top, -1 when empty
    int call_stack[VM_CALL_DEPTH]; // return addresses
    int local[VM_LOCAL_SLOT];      // local slots
    EngineStatus status;           // halted, running or waiting
    int wait_ms;                   // pause requested by the last WAIT
};
```

| Constant        | Value | Meaning                |
|-----------------|------:|------------------------|
| `VM_STACK_LEN`  | 256   | Operand stack slots    |
| `VM_LOCAL_SLOT` | 16    | Local slots            |
| `VM_CALL_DEPTH` | 8     | Maximum nested calls   |

| `EngineStatus`   | Meaning                                                    |
|------------------|------------------------------------------------------------|
| `ENGINE_HALTED`  | Stopped by `HLT` or an error. `0`, so a zeroed engine is halted |
| `ENGINE_RUNNING` | Executing instructions                                     |
| `ENGINE_WAITING` | Paused by `WAIT` for `wait_ms`, resumed by the next `vm_run` |

### Runtime

`runtime_t` is the board environment that natives act on, and the context passed to every native. The core only declares the name (`typedef struct runtime_t runtime_t;` in `engine.h`) and never reads it. The board defines its contents. On ThingBot (`board/runtime.h`):

```c
struct runtime_t {
    io_t io;          // I2C bus, SPI host, GPIO pin masks
    act_handle_t act; // PCA9685
};
```

`main` creates the buses, initializes `io` and `act`, and passes `&runtime` to `vm_init`. The VM never creates or frees hardware resources.

### Word

The only data type is the C `int`, which is 32-bit signed on the ESP32-C3. Program slots, stack entries, locals and return addresses are all `int`. Comparisons produce `1` or `0`, and `JNZ` treats any non-zero value as true.

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

| Register  | Holds                                                              |
|-----------|--------------------------------------------------------------------|
| `pc`      | Index of the next slot to fetch                                    |
| `sp`      | Index of the operand stack top, `-1` when empty                    |
| `call_sp` | Index of the call stack top, `-1` when empty                       |
| `status`  | `vm_run` steps while `ENGINE_RUNNING`, set by `HLT`, `WAIT` or error |
| `wait_ms` | Pause requested by the last `WAIT`, read by the caller of `vm_run` |

### Operand stack

`stack` holds the values that opcodes work on. It grows upward: a push writes `stack[++sp]` and a pop reads `stack[sp--]`. Arithmetic, comparison opcodes and natives pop their inputs and push their result. Bounds are not checked (see [Limitations](#limitations)).

### Locals

`local` is a single flat array of 16 slots, accessed by index with `LOAD` and `STORE`. The index is bounds-checked.

- **Global to the program:** there are no per-call frames, so a callee writing `local[idx]` overwrites the caller's value.
- **Kept between programs:** `vm_load` does not reset `local`. Since `vm` is static, it is all zeros at boot.

### Call stack

`call_stack` holds return addresses only: no arguments, locals or saved `sp`. `CALL` pushes the index of the slot after its operand and jumps; `RET` pops it back into `pc`. Overflow and underflow are checked. Arguments and return values pass through the operand stack.

## Source layout

| Module                 | Role                                                                    |
|------------------------|-------------------------------------------------------------------------|
| `vm/vm.h`, `vm.c`      | Vm facade: `vm_init`, `vm_load`, `vm_run`                               |
| `vm/engine.h`, `.c`    | Engine state, `NativeFn`, `runtime_t` name, fetch/push/pop, `engine_step` |
| `vm/opcode.h`          | `OpCode` enum, `OP_COUNT`                                               |
| `vm/handler.h/.c`      | One handler per opcode and the `op_handlers` dispatch table             |
| `vm/error.h`           | `Error` enum (not used yet)                                             |
| `board/runtime.h`      | `runtime_t` contents for ThingBot                                       |
| `board/native.h/.c`    | ThingBot natives, `NativeId` and the `thingbot_natives` table           |
| `board/io.h/.c`        | I2C bus, SPI host and GPIO pin masks                                    |
| `board/act.h/.c`       | Actuators on the PCA9685                                                |

Nothing in `vm/` includes a `board/`, ESP-IDF driver or FreeRTOS header; the dependency runs one way, from `board/` to `engine.h`. `engine.c`, `handler.c` and `native.c` use `esp_log.h` for logging.

## Lifecycle

```c
static runtime_t runtime;
static vm_t vm;

vm_init(&vm, &runtime, thingbot_natives, NATIVE_COUNT);
vm_load(&vm, prog);

while (vm_run(&vm) == ENGINE_WAITING) {
    vTaskDelay(pdMS_TO_TICKS(vm.engine.wait_ms));
}
```

### vm_init

`void vm_init(vm_t *vm, runtime_t *runtime, const NativeFn *natives, int native_count)`

Stores the runtime and the native table. Called once before the first load.

### vm_load

`void vm_load(vm_t *vm, const int *prog)`

Sets `prog` and resets the engine to its start:

- `pc = 0`, `sp = -1`, `call_sp = -1`, `status = ENGINE_RUNNING`
- `local`, the runtime and the native table are kept.

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

- `INVOKE_NATIVE` is handled by `engine_step` itself, since it is the only opcode that needs the native table and the runtime (see [Natives](#natives)).
- Every other opcode goes through `op_handlers` in `handler.c`, a table of `OpHandler` (`void (*)(engine_t *, const int *prog)`) indexed by opcode and sized `OP_COUNT`.

Before calling a handler, `engine_step` checks that:

- `0 <= opcode < OP_COUNT`
- `op_handlers[opcode]` is not `NULL`

Otherwise it logs `bad opcode` and halts. An opcode declared in `opcode.h` without a table entry fails this check.

Adding an opcode:

1. Add it to `OpCode` before `OP_COUNT`.
2. Write `static void op_<name>(engine_t *e, const int *prog)` in `handler.c`.
3. Add `[<NAME>] = op_<name>` to `op_handlers`.
4. Document it in [opcode_common.md](opcode_common.md).

Board functions are added as natives instead (see [Natives](#natives)).

## Natives

A native is a runtime function that receives the engine and the runtime:

```c
typedef void (*NativeFn)(engine_t *e, runtime_t *rt);
```

`INVOKE_NATIVE id` fetches `id`, checks `0 <= id < native_count` and that `natives[id]` is not `NULL`, then calls `natives[id](engine, runtime)`. Otherwise it logs `bad native id` and halts. A native pops its own arguments with `engine_pop`, reaches hardware through `rt`, and halts by setting `e->status = ENGINE_HALTED` on error.

The table belongs to the board. On ThingBot it is `thingbot_natives`, indexed by `NativeId` in `board/native.h`, and sized `NATIVE_COUNT`. Ids are listed in [opcode_board.md](opcode_board.md). Another board registers its own runtime and table, so the same opcodes run on any board, and only the native ids differ.

Adding a native:

1. Add it to `NativeId` before `NATIVE_COUNT`.
2. Write `static void native_<name>(engine_t *e, runtime_t *rt)` in `native.c`.
3. Add `[<NAME>] = native_<name>` to `thingbot_natives`.
4. Document it in [opcode_board.md](opcode_board.md).

## Memory

### ESP32-C3

Single-core 32-bit RISC-V at up to 160 MHz, with 400 KB of on-chip SRAM (16 KB of it reserved for the flash cache) and 8 KB of RTC SRAM. Code and read-only data run from external flash (`CONFIG_ESPTOOLPY_FLASHSIZE` is 2 MB).

On the C3, instruction RAM and data RAM share the same SRAM, so RAM-resident code, static data and the heap all come out of one pool. Current build (`idf.py size`):

| Region                    | Size      |
|---------------------------|----------:|
| Usable DRAM               | 321,296 B |
| IRAM code (`.text`)       | 41,406 B  |
| Initialized data (`.data`)| 6,925 B   |
| Zeroed data (`.bss`)      | 5,224 B   |
| Left for heap             | ~267 KB   |

Statics such as `vm` and `runtime` go in `.bss`. FreeRTOS task stacks are allocated from the heap when each task is created:

| Task       | Stack  | Config                                 |
|------------|-------:|----------------------------------------|
| `app_main` | 3584 B | `CONFIG_ESP_MAIN_TASK_STACK_SIZE`      |
| Timer      | 2048 B | `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH` |
| Idle       | 1536 B | `CONFIG_FREERTOS_IDLE_TASK_STACKSIZE`  |

Stack overflow checking is off (`CONFIG_STACK_CHECK_NONE`). To measure at runtime, use `esp_get_free_heap_size()` for the heap and `uxTaskGetStackHighWaterMark(NULL)` for the calling task's unused stack.

### VM footprint

`vm_t`, including its `engine_t`, is about 1.1 KB on the ESP32-C3:

| Part         | Size       |
|--------------|-----------:|
| `stack`      | 1024 B     |
| `local`      | 64 B       |
| `call_stack` | 32 B       |
| Other fields | ~36 B      |

The `app_main` task stack is 3584 B (`CONFIG_ESP_MAIN_TASK_STACK_SIZE`), so `vm_t` and `runtime_t` are declared `static` in `main.c`.

## Errors and halting

The engine stops when `status` becomes `ENGINE_HALTED`. It halts normally on `HLT`, and on these errors it logs with `ESP_LOGE` and halts:

| Condition            | Raised by       |
|----------------------|-----------------|
| Bad opcode           | `engine_step`   |
| Bad local index      | `LOAD`, `STORE` |
| Division by zero     | `DIV`, `MOD`    |
| Call stack overflow  | `CALL`          |
| Call stack underflow | `RET`           |
| Not implemented      | `INVOKE_STATIC` |
| Bad native id        | `INVOKE_NATIVE` |

`vm_run` returns `ENGINE_HALTED` in both cases, so the caller cannot tell a normal halt from an error. The `Error` enum in `error.h` is reserved for this and is not used yet.

## Limitations

These are not checked and lead to undefined behaviour:

- **Operand stack overflow/underflow:** pushing more than `VM_STACK_LEN` values, or popping from an empty stack.
- **Running past the end of the program:** a path that does not end with `HLT`.
- **Out-of-range jump/call targets:** `JMP`, `JNZ` and `CALL` set `pc` to any value.

Other behaviour to be aware of:

- **Integer overflow:** `ADD`, `SUB` and `MUL` are plain C `int` with no overflow handling.
- **Locals:** shared with calls and kept between programs (see [Locals](#locals)).
