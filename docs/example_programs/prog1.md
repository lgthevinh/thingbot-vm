# Multiply Until Over 1000

Starts with `x = 10` and multiplies it by 5 until it is greater than 1000. No hardware is used, so it runs on the VM alone.

## Pseudo code

```python
x = 10
while x <= 1000:
    x = x * 5
halt
```

## Program

```c
// clang-format off
const int prog[] = {
    PSH, 10,    // 0:  x = 10
    STORE, 0,   // 2:  local[0] = x
    LOAD, 0,    // 4:  loop: copy x from local[0] to stack
    PSH, 1000,
    GT,         // 8:  compare a (x) with b (1000)
    JNZ, 20,    // 9:  if not zero, jump to halt
    LOAD, 0,    // 11: GT consumed x, so get x again
    PSH, 5,
    MUL,        // 15: x * 5
    STORE, 0,   // 16: pop the value of stack into local[0]
    JMP, 4,     // 18: jump to prog[4]
    HLT,        // 20
};
// clang-format on
```

## How it works

- `x` goes 10 → 50 → 250 → 1250. At 1250, `GT` pushes `1`, and `JNZ` jumps to `HLT`.
- `JNZ` pops the condition and `GT` pops both operands, so the stack is empty at the end and `local[0]` holds `1250`.
- Locals are kept between runs (`vm_run` does not clear them), so `local[0]` is still `1250` after the program halts.

## Expected output

```
I (...) vm: halt, ip 20
```

With the log level set to debug, each step is also traced (`psh`, `gt`, `jnz`, `mul`, …).
