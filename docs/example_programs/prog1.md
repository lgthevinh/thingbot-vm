# Multiply Until Over 1000

Starts with `x = 10` and multiplies it by 5 until it is greater than 1000. No hardware is used, so it runs on the VM alone.

## Pseudo code

```python
x = 10
while x <= 1000:
    x = x * 5
halt
```

## Thingcode program

```c
// clang-format off
const int prog[] = {
    PSH, 10,    // 0:  x = 10
    GSTORE, 0,  // 2:  global[0] = x
    GLOAD, 0,   // 4:  loop: copy x from global[0] to stack
    PSH, 1000,
    GT,         // 8:  compare a (x) with b (1000)
    JNZ, 20,    // 9:  if not zero, jump to halt
    GLOAD, 0,   // 11: GT consumed x, so get x again
    PSH, 5,
    MUL,        // 15: x * 5
    GSTORE, 0,  // 16: pop the value of stack into global[0]
    JMP, 4,     // 18: jump to prog[4]
    HLT,        // 20
};
// clang-format on
```

## How it works

- `x` goes 10 → 50 → 250 → 1250. At 1250, `GT` pushes `1`, and `JNZ` jumps to `HLT`.
- `JNZ` pops the condition and `GT` pops both operands, so the stack is empty at the end and `global[0]` holds `1250`.
- Globals are only cleared by the next `vm_load`, so `global[0]` is still `1250` after the program halts.

## Expected output

```
I (...) vm: halt, pc 20
```

With the log level set to debug, each step is also traced (`psh`, `gt`, `jnz`, `mul`, …).
