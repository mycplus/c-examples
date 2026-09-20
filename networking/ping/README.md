# Ping

A minimal ICMP echo client written in C, with separate implementations for Linux and Windows.

This example demonstrates low-level network programming with ICMP echo request/reply packets, socket APIs, checksums, hostname resolution, time measurement, and platform-specific networking APIs.

## Source files

```text
ping/
├── README.md
└── src/
    ├── ping.c
    └── ping-windows.c
```

### Linux

`src/ping.c` uses POSIX sockets and the platform's ICMP definitions.

### Windows

`src/ping-windows.c` uses Winsock 2 and Windows raw sockets.

## Requirements

### Linux

- GCC or another C11-compatible compiler
- POSIX-compatible environment
- ICMP socket permissions

Build:

```bash
cc -std=c11 -Wall -Wextra -pedantic src/ping.c -o ping
```

Run:

```bash
sudo ./ping example.com
```

An unprivileged ICMP datagram socket is attempted first. If that is unavailable, the program falls back to a raw socket and reports the required permissions.

You can optionally specify the payload size:

```bash
sudo ./ping example.com 128
```

The valid payload range is 0–1024 bytes.

### Windows

- Visual Studio / MSVC
- Developer Command Prompt for Visual Studio
- Administrator privileges for the raw ICMP socket

Build:

```bat
cl /nologo /TC /W4 /WX /EHsc src\ping-windows.c /link Ws2_32.lib
```

Run from an elevated command prompt:

```bat
ping-windows.exe example.com
```

Optional payload size:

```bat
ping-windows.exe example.com 128
```

## Example output

The exact output depends on the host and network:

```text
PING example.com (93.184.216.34) 56 bytes of data [unprivileged socket]
64 bytes from 93.184.216.34: icmp_seq=0 time=...
64 bytes from 93.184.216.34: icmp_seq=1 time=...
64 bytes from 93.184.216.34: icmp_seq=2 time=...
64 bytes from 93.184.216.34: icmp_seq=3 time=...
--- 93.184.216.34 statistics ---
4 sent, 4 received, 0% loss
```

## Platform differences

The two source files are intentionally separate because the networking APIs and ICMP header definitions differ between POSIX systems and Windows.

- Linux uses POSIX socket APIs and system-provided IP/ICMP headers.
- Windows uses Winsock 2 and declares the required IP and ICMP header structures locally.
- The Windows implementation requires an elevated process because it uses a raw ICMP socket.

## Automated builds

GitHub Actions checks that both implementations compile with warnings treated as errors:

- GCC on Ubuntu
- Clang on Ubuntu
- MSVC on Windows

The workflow performs compile-time validation and a safe argument-validation smoke test. It does not attempt to send ICMP packets from GitHub-hosted runners, because network access and raw-socket privileges are environment-dependent.

## Related MYCPLUS content

This project accompanies the relevant C programming tutorial on [MYCPLUS.com](https://www.mycplus.com/).
