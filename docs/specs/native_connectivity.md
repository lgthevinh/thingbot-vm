# Connectivity Native Specification

**Draft.** Common natives for networking and wireless, in the connectivity range `0x1000 .. 0x1FFF` (see [native_common.md](native_common.md#id-ranges)). They are not implemented yet. They depend on:

- **Linear memory:** implemented, buffers point into it (see [Buffers](#buffers)).
- **Module lookup:** not implemented yet; the current lookup cannot register natives at `0x1000` (see [native_common.md](native_common.md#modules)).

Each module is optional. A runtime without the hardware does not register it, and calling its natives halts with `bad native id`.

## Table of Contents

- [Conventions](#conventions)
  - [Results](#results)
  - [Polling](#polling)
  - [Handles](#handles)
  - [Buffers](#buffers)
  - [Addresses](#addresses)
- [TCP](#tcp)
- [UDP](#udp)
- [BLE](#ble)
- [Open questions](#open-questions)

## Conventions

### Results

Every connectivity native pushes a result `r` as its top output. This convention covers the connectivity modules only: other modules, such as ThingBot's, define their own outputs. `r >= 0` is success: a handle, a byte count or a state. `r < 0` is an error code:

| Value | Name             | Meaning                                              |
|------:|------------------|------------------------------------------------------|
| -1    | `ERR_FAIL`       | The operation failed (refused, unreachable, timeout) |
| -2    | `ERR_AGAIN`      | Not ready yet, call again later                      |
| -3    | `ERR_CLOSED`     | The connection is closed                             |
| -4    | `ERR_BAD_HANDLE` | The handle is not open in this module                |
| -5    | `ERR_NO_HANDLE`  | No free handle left                                  |

Network failures are normal on a robot, so they are returned as results and the program decides what to do. Programming errors halt the VM, as in other natives:

- a buffer outside linear memory, or a negative length
- a port outside `0 .. 65535`
- a flag value that is not defined

### Polling

No native blocks. A native that cannot complete yet returns `ERR_AGAIN`, or a state that is not final, and the program polls with `WAIT` in between:

```
// wait until a TCP connection is up
PSH, ip, PSH, port, INVOKE_NATIVE, TCP_CONNECT, GSTORE, 0,
loop:
  PSH, 50, WAIT,
  GLOAD, 0, INVOKE_NATIVE, TCP_STATUS,
  PSH, TCP_CONNECTING, EQ, JNZ, loop
```

### Handles

Sockets, listeners and BLE objects are identified by handles: small non-negative ints owned by the runtime. A handle is valid only in the module that created it. The number of open handles is limited by the runtime.

Handles have one of two owners:

- **Program handles** are opened by the program's natives (`TCP_CONNECT`, `UDP_OPEN`, …). The runtime closes every program handle the program left open before the next `vm_load`.
- **Runtime handles** are opened by the runtime itself, e.g. a desktop runtime's connection to the editor. They survive `vm_load`, and a program cannot close them: `*_CLOSE` on one returns `ERR_BAD_HANDLE`.

### Buffers

Data is passed as a `ptr len` pair: a byte range `ptr .. ptr + len - 1` in the engine's linear memory. A native reads from it or writes into it. Strings, such as a BLE device name, are UTF-8 bytes in a buffer with no terminator.

Linear memory is `VM_MEM_LEN` (4096) bytes, cleared on every `vm_load` (see [vm.md](vm.md#linear-memory)). A program fills it with `MSTORE8` / `MSTORE32` (see [opcode.md](opcode.md#memory)), including constants such as names and UUIDs. An out-of-range `ptr len` halts the VM.

### Addresses

An IPv4 address is one int with the first octet in the high byte: `a.b.c.d` is `a << 24 | b << 16 | c << 8 | d`, e.g. `192.168.1.10` is `0xC0A8010A`. The value is a bit pattern, so addresses from `128.0.0.0` up are negative ints. Hostnames are not supported.

Joining a network, e.g. Wi-Fi credentials, is done by the runtime before the program runs, not by a native.

## TCP

Module `0x10`, ids `0x1000 .. 0x10FF`.

| Id       | Name          | Stack effect         | Result                                                                            |
|---------:|---------------|----------------------|-----------------------------------------------------------------------------------|
| `0x1000` | `TCP_CONNECT` | `( ip port -- r )`   | Handle of a connection being opened                                               |
| `0x1001` | `TCP_LISTEN`  | `( port -- r )`      | Handle of a listener on `port`                                                    |
| `0x1002` | `TCP_ACCEPT`  | `( h -- r )`         | Handle of the next incoming connection, or `ERR_AGAIN`                            |
| `0x1003` | `TCP_STATUS`  | `( h -- r )`         | `TCP_CONNECTING` (0), `TCP_CONNECTED` (1), or an error                            |
| `0x1004` | `TCP_SEND`    | `( h ptr len -- r )` | Bytes queued, may be less than `len`, or `ERR_AGAIN` when the send buffer is full |
| `0x1005` | `TCP_RECV`    | `( h ptr len -- r )` | Bytes read, `ERR_AGAIN` when none yet, `ERR_CLOSED` when the peer closed          |
| `0x1006` | `TCP_CLOSE`   | `( h -- r )`         | `0`, and the handle is freed                                                      |

- `TCP_CONNECT` returns at once. A failed connection shows up in `TCP_STATUS` as `ERR_FAIL`.
- TCP is a byte stream: one `TCP_SEND` can arrive as several `TCP_RECV`, and the other way round.
- A handle stays allocated after an error until `TCP_CLOSE`.

## UDP

Module `0x11`, ids `0x1100 .. 0x11FF`.

| Id       | Name        | Stack effect                 | Result                                            |
|---------:|-------------|------------------------------|---------------------------------------------------|
| `0x1100` | `UDP_OPEN`  | `( port -- r )`              | Handle bound to local `port`, `0` for any port    |
| `0x1101` | `UDP_SEND`  | `( h ip port ptr len -- r )` | `len` when the datagram is sent                   |
| `0x1102` | `UDP_RECV`  | `( h ptr len -- ip port r )` | Bytes of the next datagram, `ERR_AGAIN` when none |
| `0x1103` | `UDP_CLOSE` | `( h -- r )`                 | `0`, and the handle is freed                      |

- One `UDP_SEND` is one datagram. A datagram longer than `len` is truncated by `UDP_RECV`.
- `UDP_RECV` always pushes three values. `ip` and `port` are the sender, or `0` when `r` is an error.
- Delivery and order are not guaranteed.

## BLE

Module `0x12`, ids `0x1200 .. 0x12FF`. Peripheral role only: the robot advertises and a central, such as a phone, connects to it.

| Id       | Name            | Stack effect                 | Result                                                                |
|---------:|-----------------|------------------------------|-----------------------------------------------------------------------|
| `0x1200` | `BLE_SERVICE`   | `( uuid_ptr -- r )`          | Handle of a new GATT service                                          |
| `0x1201` | `BLE_CHAR`      | `( s uuid_ptr props -- r )`  | Handle of a new characteristic in service `s`                         |
| `0x1202` | `BLE_START`     | `( name_ptr name_len -- r )` | `0` when advertising as `name`                                        |
| `0x1203` | `BLE_STOP`      | `( -- r )`                   | `0`, advertising stopped and centrals disconnected                    |
| `0x1204` | `BLE_CONNECTED` | `( -- r )`                   | Number of connected centrals                                          |
| `0x1205` | `BLE_WRITE`     | `( c ptr len -- r )`         | `0` when the value of `c` is set                                      |
| `0x1206` | `BLE_READ`      | `( c ptr len -- r )`         | Bytes of the value a central last wrote, `ERR_AGAIN` when nothing new |

- `uuid_ptr` points to a 128-bit UUID, 16 bytes in linear memory, most significant byte first.
- `props` is a bit mask: `BLE_READ_PROP` (1) lets a central read, `BLE_WRITE_PROP` (2) lets it write, `BLE_NOTIFY_PROP` (4) lets it subscribe.
- Services and characteristics are defined before `BLE_START`. Defining them after returns `ERR_FAIL`.
- `BLE_WRITE` on a characteristic with `BLE_NOTIFY_PROP` also notifies subscribed centrals.
- `BLE_READ` returns each write once: after it is read, the next `BLE_READ` returns `ERR_AGAIN` until a central writes again.

## Open questions

1. **Module lookup:** registering natives by module instead of one dense common table (see [native_common.md](native_common.md#modules)).
2. **Cleanup between programs:** closing program handles before the next `vm_load` (see [Handles](#handles)) needs a reset hook from the VM to the runtime.
3. **Network join:** whether Wi-Fi setup stays runtime configuration or becomes its own module.
4. **Hostnames:** a DNS native, taking the name as a buffer.
5. **BLE scope:** central role, and 16-bit UUIDs to avoid a buffer for standard services.
6. **Constant data:** names and UUIDs are written into memory with one `MSTORE32` per word today. A cheaper way to load constants, such as a data section, is not decided.
