# TCP Echo Client and Server

A TCP echo server and client in C using POSIX sockets. The server handles
each client in its own thread and returns every byte it receives. The
client sends a message, reads the echo back and checks that it matches.

## Source files

```text
tcp-echo/
├── CMakeLists.txt
├── README.md
├── include/
│   └── net.h             send_all and recv_exact
└── src/
    ├── net.c             byte-stream helpers
    ├── tcp-server.c      echo server, one thread per client
    └── tcp-client.c      echo client
```

## What the code handles

- **TCP is a byte stream.** One `send()` is not guaranteed to arrive as one
  `recv()`. The client reads until it has exactly as many bytes as it sent.
- **Partial sends.** `send()` may write fewer bytes than requested;
  `send_all` loops until everything is written.
- **Interrupted calls.** `EINTR` is retried rather than treated as an error.
- **Closed peers.** `recv()` returning 0 is an orderly shutdown. `SIGPIPE`
  is ignored so writing to a closed socket reports `EPIPE`.
- **Restarts.** `SO_REUSEADDR` lets the server rebind while old connections
  are in `TIME_WAIT`.
- **IPv4 and IPv6.** Addresses are resolved with `getaddrinfo`. The server
  prefers a dual-stack IPv6 socket and falls back to IPv4.
- **Binary data.** Data is handled as bytes with explicit lengths; nothing
  assumes a NUL terminator.

## Build

### CMake

```bash
cmake -S . -B build
cmake --build build
```

### Directly

```bash
cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/net.c src/tcp-server.c -o tcp-server -lpthread
cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/net.c src/tcp-client.c -o tcp-client
```

## Run

```bash
./tcp-server 5150
```

In another terminal:

```bash
./tcp-client localhost 5150 "Hello, TCP"
sent and verified 1 message(s) of 10 bytes

./tcp-client localhost 5150 "ping" 1000
sent and verified 1000 message(s) of 4 bytes
```

## Platform

This is POSIX code for Linux and macOS. It does not build on Windows, where
sockets come from Winsock and differ in initialisation, types and error
reporting. See the Windows sockets article on MYCPLUS for those differences.

## Limitations

- **One thread per client.** Simple and clear, but each connection costs a
  thread. Servers handling many connections use `poll`, `epoll` or `kqueue`.
- **Send, then read.** The client sends a whole message before reading its
  echo. That is fine while a message fits in the socket buffers; for large
  transfers, send and receive concurrently or in alternating chunks.

## Automated builds

GitHub Actions builds with GCC and Clang on Ubuntu and Apple Clang on macOS,
warnings treated as errors, then starts the server and runs the client
against it over IPv4 and IPv6, including concurrent clients and a message
larger than the server's buffer. Separate jobs run the pair under
AddressSanitizer and UndefinedBehaviorSanitizer, and the server under
ThreadSanitizer.
