# thingbot-vm

A small stack-based virtual machine for ThingBot, running on an ESP32-C3 (WT32C3-S5) with ESP-IDF.

## Table of Contents

- [Docs](#docs)
- [Build](#build)
- [License](#license)

## Docs

| Doc                                 | Covers                                                           |
|-------------------------------------|------------------------------------------------------------------|
| [VM spec](docs/specs/vm.md)         | Architecture, lifecycle, dispatch, board context, memory, errors |
| [Opcode spec](docs/specs/opcode.md) | Every opcode: operand, stack effect, behaviour, errors           |

## Build

Requires ESP-IDF v6.0.

```sh
idf.py set-target esp32c3
idf.py build flash monitor
```

## License

GPL-3.0. See [LICENSE](LICENSE).
