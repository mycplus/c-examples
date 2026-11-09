# Serial Chat: PC to PC Communication in C

[![serial-chat](https://github.com/mycplus/c-examples/actions/workflows/serial-chat.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/serial-chat.yml)

Companion code for [PC to PC Communication in C: Serial Port Chat on Linux and Windows](https://www.mycplus.com/programming/c/pc-to-pc-communication/) on MYCPLUS.

Two computers joined by a three-wire null-modem cable (or a pair of USB-to-serial adapters) exchange text messages. Each message is framed with SLIP (RFC 1055) and protected by a CRC-16, so a receiver can find where messages start and end and drop damaged ones.

| File | What it is |
| --- | --- |
| `include/serial.h` | A four-function serial port interface: open, write, read with timeout, close |
| `src/serial_posix.c` | That interface on Linux and macOS, using termios in raw mode |
| `src/serial_win32.c` | The same interface on Windows, using `CreateFile`, `SetCommState` and `COMMTIMEOUTS` |
| `include/frame.h`, `src/frame.c` | SLIP framing with CRC-16/CCITT-FALSE, and a byte-at-a-time decoder |
| `src/chat.c` | The chat program: `chat /dev/ttyUSB0 115200` or `chat COM3 115200` |
| `src/frame_demo.c` | Escaping, and what the decoder does with damaged and noisy input |
| `src/crc_experiment.c` | Damages 2.6 million frames on purpose and counts how many wrong messages get through |
| `src/baud_table.c` | What each baud rate means in bytes per second |
| `src/pty_loopback.c` | Runs the serial code against a pseudo-terminal pair, no cable needed (Linux, macOS) |
| `pitfalls/cooked_mode.c` | What a terminal device's default mode does to binary data |
| `pitfalls/dos_bioscom.c` | DOS-era Turbo C serial code, kept to show that it no longer builds |
| `tests/` | Unit tests, a two-process chat test over `socat`, and the expected output of every demo |
| `run_all.sh`, `run_all.ps1` | One-command verification for Ubuntu/WSL and for Windows (MSVC, C11 and C17); each prints a Markdown table |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For the two-terminal chat test on Linux, install `socat` first (`sudo apt install socat`). To check everything in one go: `sh run_all.sh` on Ubuntu or WSL, `.\run_all.ps1` from a Developer PowerShell on Windows.

## Try the chat without hardware

On Linux, `socat` creates two connected pseudo-terminals, which behave like two serial ports joined by a cable:

```sh
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
./build/chat /tmp/ttyA      # in a second terminal
./build/chat /tmp/ttyB      # in a third terminal
```

On Windows, a virtual null-modem driver such as com0com creates a connected pair of COM ports.

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, with MSVC under `/W4 /WX` in C11 and C17, and cross-compiles the Windows build with MinGW-w64.
- The CRC matches the published check value (0x29B1 for "123456789") and an independent table-driven implementation over 2,000 random inputs.
- Every payload length from 1 to 256 bytes, with random bytes and with payloads made entirely of END or ESC bytes, survives an encode and decode round trip, and END never appears inside a frame.
- Every possible single-bit error in a 32-byte frame is rejected; oversized frames, bad escapes, empty frames and stray bytes are rejected and the decoder recovers at the next frame. The size limit is checked at exactly 256 and 257 bytes.
- On Linux: the serial code passes a message each way through a pseudo-terminal with raw mode intact, the read timeout works, and two `chat` processes exchange a line each way over a `socat` cable.
- Every demo prints exactly the output shown in the article, and the DOS-era code still fails to build.
- The tests and demos run clean under AddressSanitizer and UndefinedBehaviorSanitizer.

The build does **not** test a physical serial port or a real COM port on Windows (no hardware in CI), or baud-rate accuracy, which pseudo-terminals ignore.
