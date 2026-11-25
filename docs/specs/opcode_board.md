# Board Native Specification

ThingBot natives that drive the actuators on the PCA9685, called with [`INVOKE_NATIVE id`](opcode_common.md#invoke_native). They follow the conventions in [opcode_common.md](opcode_common.md). Native ids are defined by `NativeId` in `main/board/native.h`, implemented in `main/board/native.c`, and reach the hardware through the `act_*` functions in `main/board/act.c` via the board context (`thingbot_t *bot = vm->ctx`). `main` registers the table with `vm_init(&vm, &bot, thingbot_natives, NATIVE_COUNT)`.

## Table of Contents

- [Conventions](#conventions)
- [Native Summary](#native-summary)
- [Channel Map](#channel-map)
- [Actuators](#actuators)
  - [MOTOR](#motor), [SERVO](#servo), [SPIN](#spin), [BUZZ](#buzz), [LED](#led)
- [Errors](#errors)

## Conventions

- All arguments come from the stack; the only inline operand is the native id of `INVOKE_NATIVE`. Use `PSH` to supply constants.
- When a native takes a device `id`, push the `id` first and the value last (value on top).
- Device ids are 1-based and match the board labels: `id` 1 is `M1`, `SERVO_1`, `LED_1`.

```
PSH, 1, PSH, 80, INVOKE_NATIVE, MOTOR,   // motor M1 at speed 80
```

## Native Summary

| Id | Name    | Stack effect           | Range       | Calls                 |
|---:|---------|------------------------|-------------|-----------------------|
| 0  | `MOTOR` | `( id speed -- )`      | -100 .. 100 | `act_set_motor_speed` |
| 1  | `SERVO` | `( id angle -- )`      | 0 .. 180    | `act_set_servo_angle` |
| 2  | `SPIN`  | `( id speed -- )`      | -100 .. 100 | `act_set_servo_speed` |
| 3  | `BUZZ`  | `( duty -- )`          | 0 .. 100    | `act_set_buzzer`      |
| 4  | `LED`   | `( id brightness -- )` | 0 .. 100    | `act_set_led`         |

Values outside the range are clamped, not rejected.

## Channel Map

PCA9685 channels used by each device.

| Device  | id | Channel(s) |
|---------|---:|------------|
| M1      | 1  | A 2, B 3   |
| M2      | 2  | A 4, B 5   |
| M3      | 3  | A 6, B 7   |
| M4      | 4  | A 1, B 0   |
| SERVO_1 | 1  | 12         |
| SERVO_2 | 2  | 11         |
| SERVO_3 | 3  | 10         |
| SERVO_4 | 4  | 9          |
| SERVO_5 | 5  | 8          |
| BUZZER  | -  | 14         |
| LED_1   | 1  | 15         |
| LED_2   | 2  | 13         |

## Actuators

All channels run at one PWM frequency, 50 Hz, set in `act_init` (the PCA9685 has a single prescaler). Duty is 12-bit, `0 .. 4095`.

### MOTOR

Sets the speed and direction of a DC motor.

- Stack effect: `( id speed -- )`
- `id`: `1 .. 4`
- `speed`: `-100 .. 100`. Positive drives channel A, negative drives channel B, with duty `|speed| * 4095 / 100`. `0` sets both low (coast).
- Error: `id` out of range halts the VM.

### SERVO

Sets the angle of a 180° servo.

- Stack effect: `( id angle -- )`
- `id`: `1 .. 5`
- `angle`: `0 .. 180`, mapped linearly to a `500 .. 2500` µs pulse.
- Error: `id` out of range halts the VM.

### SPIN

Sets the speed and direction of a 360° (continuous rotation) servo, on the same ports as `SERVO`.

- Stack effect: `( id speed -- )`
- `id`: `1 .. 5`
- `speed`: `-100 .. 100`, mapped to a `1000 .. 2000` µs pulse. `0` is `1500` µs (stop).
- The stop point varies between servos; adjust `SPIN_STOP_US` in `act.c` if one creeps at `0`.
- Error: `id` out of range halts the VM.

### BUZZ

Sets the buzzer duty.

- Stack effect: `( duty -- )`
- `duty`: `0 .. 100` percent. The PWM frequency is fixed at 50 Hz, so the duty changes loudness, not pitch.

### LED

Sets the brightness of an LED.

- Stack effect: `( id brightness -- )`
- `id`: `1 .. 2`
- `brightness`: `0 .. 100` percent, `0` is off.
- Error: `id` out of range halts the VM.

## Errors

Natives follow the halting behavior in [opcode_common.md](opcode_common.md#current-behavior). A native logs and halts when its `act_*` call fails. A native id outside the table is rejected by `INVOKE_NATIVE` before any native runs:

| Condition       | Trigger                                                               |
|-----------------|
| Bad device id   | `id` outside the device range (`act_*` returns `ESP_ERR_INVALID_ARG`) |
| Actuator failed | Any other non-`ESP_OK` return from `act_*` (e.g. I2C error)           |
