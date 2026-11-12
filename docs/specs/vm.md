# Virtual machine Specifications

Stack-based VM running on ThingBot (ESP32-C3). The core (`vm.c`) has no hardware dependencies; board-specific code lives in `main/board/` and reaches the board opcode handlers through a context pointer. Opcode semantics are specified in [opcode_common.md](opcode_common.md).

## Table of Contents

- [Architecture](#architecture)
- [Lifecycle](#lifecycle)
  - [vm_init](#vm_init)
  - [vm_run](#vm_run)
- [Execution loop](#execution-loop)
- [Dispatch](#dispatch)
- [Board context](#board-context)
- [Memory](#memory)
  - [ESP32-C3](#esp32-c3)
  - [VM footprint](#vm-footprint)
- [Errors and halting](#errors-and-halting)
- [Limitations](#limitations)

## Architecture

| Module                 | Role                                                            |
|------------------------|-----------------------------------------------------------------|
| `vm/vm.h`, `vm.c`      | VM state, `vm_init`, `vm_run`, fetch/push/pop helpers, dispatch |
| `vm/opcode.h`          | `OpCode` enum, `OP_COUNT`                                       |
| `vm/handler.h/.c`      | One handler per opcode and the `op_handlers` dispatch table     |
| `vm/error.h`           | `Error` enum (not used yet)                                     |
| `board/thingbot.h`     | `thingbot_t` board context                                      |
| `board/io.h/.c`        | I2C bus, SPI host and GPIO pin masks                            |
| `board/act.h/.c`       | Actuators on the PCA9685                                        |

`vm.c` includes no ESP-IDF driver headers. `handler.c` includes `thingbot.h` for the board opcodes, so it depends on `board` and its drivers. Only `vm.c` and `handler.c` use `esp_log.h` for logging.

## Lifecycle

```c
static thingbot_t bot;
static vm_t vm;

vm_init(&vm, &bot);
vm_run(&vm, prog);
```

### vm_init

`void vm_init(vm_t *vm, void *ctx)`

Stores the board context. Called once before the first run.

### vm_run

`void vm_run(vm_t *vm, const int *prog)`

Resets the VM and runs `prog` until it halts:

- `prog = prog`, `ip = 0`, `sp = -1`, `call_sp = -1`, `running = true`
- `local` and `ctx` are kept.

`vm_run` blocks until the program halts. The same `vm_t` can run several programs one after another.

## Execution loop

```
while running:
    opcode = prog[ip++]
    dispatch(opcode)
```

- Handlers that take an operand fetch it with `vm_fetch`, which also advances `ip`.
- After a handler returns, `ip` points at the next instruction unless the handler changed it (`JMP`, `JNZ`, `CALL`, `RET`).
- Log lines report `ip` as the index of the instruction being executed: `ip - 1` for opcodes without an operand, `ip - 2` for opcodes with one.

## Dispatch

`op_handlers` in `handler.c` is a table of `OpHandler` (`void (*)(vm_t *)`) indexed by opcode and sized `OP_COUNT`.

Before calling a handler, `vm_exec` checks that:

- `0 <= opcode < OP_COUNT`
- `op_handlers[opcode]` is not `NULL`

Otherwise it logs `bad opcode` and halts. Opcodes declared in `opcode.h` without a table entry (the ThingBot opcodes `SET_BUZ` .. `GET_SW` today) fail this check.

Adding an opcode:

1. Add it to `OpCode` before `OP_COUNT`.
2. Write `static void op_<name>(vm_t *vm)` in `handler.c`.
3. Add `[<NAME>] = op_<name>` to `op_handlers`.
4. Document it in [opcode_common.md](opcode_common.md), or [opcode_board.md](opcode_board.md) for board opcodes.

## Board context

`vm_t.ctx` points to the board context, which the VM core never reads. On ThingBot it is a `thingbot_t`:

```c
typedef struct {
    io_t io;          // I2C bus, SPI host, GPIO pin masks
    act_handle_t act; // PCA9685
} thingbot_t;
```

Handlers that need hardware cast it back:

```c
thingbot_t *bot = vm->ctx;
```

`main` creates the buses, initializes `io` and `act`, and passes `&bot` to `vm_init`. The VM never creates or frees hardware resources.

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

Statics such as `vm` and `bot` go in `.bss`. FreeRTOS task stacks are allocated from the heap when each task is created:

| Task       | Stack  | Config                                 |
|------------|-------:|----------------------------------------|
| `app_main` | 3584 B | `CONFIG_ESP_MAIN_TASK_STACK_SIZE`      |
| Timer      | 2048 B | `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH` |
| Idle       | 1536 B | `CONFIG_FREERTOS_IDLE_TASK_STACKSIZE`  |

Stack overflow checking is off (`CONFIG_STACK_CHECK_NONE`). To measure at runtime, use `esp_get_free_heap_size()` for the heap and `uxTaskGetStackHighWaterMark(NULL)` for the calling task's unused stack.

### VM footprint

`vm_t` is about 1.1 KB on the ESP32-C3:

| Part         | Size       |
|--------------|-----------:|
| `stack`      | 1024 B     |
| `local`      | 64 B       |
| `call_stack` | 32 B       |
| Other fields | ~24 B      |

The `app_main` task stack is 3584 B (`CONFIG_ESP_MAIN_TASK_STACK_SIZE`), so `vm_t` and `thingbot_t` are declared `static` in `main.c`. As statics, `local` starts as all zeros at boot.

## Errors and halting

The VM stops when `running` becomes `false`. It halts normally on `HLT`, and on these errors it logs with `ESP_LOGE` and halts:

| Condition            | Raised by       |
|----------------------|-----------------|
| Bad opcode           | `vm_exec`       |
| Bad local index      | `LOAD`, `STORE` |
| Call stack overflow  | `CALL`          |
| Call stack underflow | `RET`           |

`vm_run` returns nothing, so the caller cannot tell a normal halt from an error. The `Error` enum in `error.h` is reserved for this and is not used yet.

## Limitations

These are not checked and lead to undefined behaviour:

- **Operand stack overflow/underflow:** pushing more than `VM_STACK_LEN` values, or popping from an empty stack.
- **Running past the end of the program:** a path that does not end with `HLT`.
- **Out-of-range jump/call targets:** `JMP`, `JNZ` and `CALL` set `ip` to any value.

Other behaviour to be aware of:

- **Locals persist between runs:** `vm_run` does not reset `local`.
- **Locals are shared with calls:** a callee writing `local[idx]` overwrites the caller's value.
- **Integer overflow:** arithmetic is plain C `int` with no overflow handling.
