# thingbot-vm

A small stack-based virtual machine for ThingBot that runs Thingcode programs, on an ESP32-C3 (WT32C3-S5) with ESP-IDF.

## Table of Contents

- [Docs](#docs)
- [Build](#build)
- [License](#license)

## Docs

| Doc                                                           | Covers                                                                    |
|---------------------------------------------------------------|---------------------------------------------------------------------------|
| [VM spec](docs/specs/vm.md)                                   | Vm / Engine / Runtime split, lifecycle, dispatch, natives, memory, errors |
| [Opcode spec](docs/specs/opcode.md)                           | Thingcode opcodes: operand, stack effect, behaviour, errors               |
| [Common native spec](docs/specs/native_common.md)             | Native id ranges, modules, module registry, core natives                  |
| [Connectivity native spec](docs/specs/native_connectivity.md) | Draft TCP, UDP and BLE natives                                            |
| [ThingBot native spec](docs/specs/native_thingbot.md)         | ThingBot natives for `INVOKE_NATIVE`, PCA9685 channel map                 |

## Build

Requires ESP-IDF v6.0.

```sh
idf.py set-target esp32c3
idf.py build flash monitor
```

## License

GPL-3.0. See [LICENSE](LICENSE).
