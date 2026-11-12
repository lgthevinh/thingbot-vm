# Drive and Scan

Drives forward for 1 s on motors M1 and M2, stops, sweeps servo 1 from 0° to 180° in 30° steps, centers it at 90°, then beeps once.

## Pseudo code

```python
motor(1, 60)
motor(2, 60)
wait(1000)
stop()

angle = 0
while angle <= 180:
    servo(1, angle)
    wait(300)
    angle = angle + 30

servo(1, 90)
buzz(50)
wait(100)
buzz(0)
halt

def stop():
    motor(1, 0)
    motor(2, 0)
```

## Program

```c
// clang-format off
const int prog[] = {
    PSH, 1,      // 0:  M1 forward at 60%
    PSH, 60,
    MOTOR,
    PSH, 2,      // 5:  M2 forward at 60%
    PSH, 60,
    MOTOR,
    PSH, 1000,   // 10: drive for 1 s
    WAIT,
    CALL, 56,    // 13: stop both motors
    PSH, 0,      // 15: angle = 0
    STORE, 0,
    PSH, 1,      // 19: loop: servo 1 to angle
    LOAD, 0,
    SERVO,
    PSH, 300,    // 24: let the servo reach it
    WAIT,
    LOAD, 0,     // 27: angle += 30
    PSH, 30,
    ADD,
    STORE, 0,
    LOAD, 0,     // 34: loop while angle <= 180
    PSH, 180,
    LTE,
    JNZ, 19,
    PSH, 1,      // 41: center servo 1
    PSH, 90,
    SERVO,
    PSH, 50,     // 46: beep 100 ms
    BUZZ,
    PSH, 100,
    WAIT,
    PSH, 0,      // 52: buzzer off
    BUZZ,
    HLT,         // 55

    // stop: both motors to 0 (coast)
    PSH, 1,      // 56
    PSH, 0,
    MOTOR,
    PSH, 2,      // 61
    PSH, 0,
    MOTOR,
    RET,         // 66
};
// clang-format on
```

## How it works

- The stop routine lives after `HLT` at ip 56 and is reached only through `CALL`. `RET` returns to ip 15.
- `local[0]` holds the angle. The loop runs for 0, 30, … 180 (7 positions), then exits when the angle reaches 210.
- If a motor turns the wrong way, swap its A/B pair in `motor_ch` in `act.c`.

## Expected output

Not yet run on hardware. The VM should halt after about 3.2 s (1000 ms drive + 7 × 300 ms sweep + 100 ms beep):

```
I (...) vm: halt, ip 55
```
