# Beep and Blink

Beeps the buzzer and blinks LED 1 three times, 200 ms on and 200 ms off.

Opcodes: `PSH`, `LOAD`, `STORE`, `SUB`, `JNZ`, `LED`, `BUZZ`, `WAIT`, `HLT`.

## Program

```c
// clang-format off
const int prog[] = {
    PSH, 3,    // 0:  count = 3
    STORE, 0,  // 2:  local[0] = count
    PSH, 1,    // 4:  loop: LED 1 on
    PSH, 100,
    LED,
    PSH, 50,   // 9:  buzzer on, 50% duty
    BUZZ,
    PSH, 200,  // 12: wait 200 ms
    WAIT,
    PSH, 1,    // 15: LED 1 off
    PSH, 0,
    LED,
    PSH, 0,    // 20: buzzer off, outputs hold their last value after HLT
    BUZZ,
    PSH, 200,  // 23: wait 200 ms
    WAIT,
    LOAD, 0,   // 26: count - 1
    PSH, 1,
    SUB,
    STORE, 0,
    LOAD, 0,   // 33: loop while count != 0
    JNZ, 4,
    HLT,       // 37
};
// clang-format on
```

## How it works

- `local[0]` holds the loop counter. `JNZ` pops the condition, so the counter is loaded again after `STORE` to test it.
- The PCA9685 keeps its last output after `HLT`, so the buzzer and LED are turned off before the program ends.

## Expected output

The VM halts about 1.2 s after start (6 × 200 ms):

```
I (253) main_task: Calling app_main()
I (1463) vm: halt, ip 37
```
