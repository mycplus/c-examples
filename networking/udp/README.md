# UDP Sender and Receiver in C

[![udp](https://github.com/mycplus/c-examples/actions/workflows/udp.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/udp.yml)

Companion code for [UDP Sender and Receiver in C: sendto, recvfrom and Lost Datagrams](https://www.mycplus.com/programming/c/udp-sender-and-receiver/) on MYCPLUS. The same source builds with BSD sockets on Linux and macOS and with Winsock 2 on Windows.

| File | What it is |
| --- | --- |
| `include/udp.h`, `src/udp.c` | A thin portability layer: start-up, closing, errors, timeouts, binding (IPv6 dual-stack or IPv4) and address resolution |
| `src/udp-sender.c` | Sends numbered datagrams to a host and port: `udp-sender HOST PORT MESSAGE [COUNT] [DELAY_MS]` |
| `src/udp-receiver.c` | Receives datagrams on a port and reports gaps in their sequence numbers: `udp-receiver PORT [COUNT] [TIMEOUT_MS]` |
| `src/boundaries.c` | Message boundaries, a zero-length datagram, and what a too-small buffer does |
| `src/connected.c` | Sending to a closed port with an unconnected and a connected socket |
| `src/loss_test.c` | Sends thousands of datagrams to a socket that is not reading yet and counts the survivors |
| `tests/` | The expected output of `boundaries` and `connected`, and a sender/receiver test over IPv4 and IPv6 loopback |
| `run_all.sh`, `run_all.ps1` | One-command verification for Ubuntu/WSL and Windows (MSVC, C11 and C17); each prints a Markdown table |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Try it in two terminals:

```sh
./build/udp-receiver 5150
./build/udp-sender 127.0.0.1 5150 "boiler temp=71.3C" 5
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX` in C11 and C17.
- `boundaries` and `connected` print exactly the output shown in the article.
- `loss_test` checks invariants that hold on every system: some datagrams arrive, none is corrupted or duplicated, and none arrives out of order on loopback. The number lost depends on the system and is not checked.
- On Linux and macOS: the receiver gets datagrams from IPv4 and IPv6 senders and reports a gap in the sequence numbers.
- The programs and the sender/receiver test run clean under AddressSanitizer and UndefinedBehaviorSanitizer.

The build does **not** test across a real network, where loss, reordering and duplication come from routers and links rather than a full receive buffer.
